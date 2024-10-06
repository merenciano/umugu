#include "umugu.h"
#include "umugu_internal.h"
#include "nodes/builtin_nodes.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#define UMUGU_HEAD_CODE 0x00454549
#define UMUGU_TAIL_CODE 0x00555541

#define UMUGU_GET_VERSION \
    ((UMUGU_VERSION_MAJOR << 20) | \
     (UMUGU_VERSION_MINOR << 10) | \
      UMUGU_VERSION_PATCH)


static umugu_ctx g_default_ctx; /* Relying on zero-init from static memory. */
static umugu_ctx *g_ctx = &g_default_ctx;

umugu_node_info g_builtin_nodes_info[UM__BUILTIN_NODES_COUNT];

static inline int um__node_info_builtin_load(void)
{
    g_builtin_nodes_info[0] = (umugu_node_info) {
        .name = {.str = "Oscilloscope"},
        .size_bytes = um__oscope_size, 
        .var_count = um__oscope_var_count,
        .getfn = um__oscope_getfn, 
        .vars = um__oscope_vars,
        .plug_handle = NULL
    };

    g_builtin_nodes_info[1] = (umugu_node_info) {
        .name = {.str = "WavFilePlayer"},
        .size_bytes = um__wavplayer_size,
        .var_count = um__wavplayer_var_count,
        .getfn = um__wavplayer_getfn,
        .vars = um__wavplayer_vars,
        .plug_handle = NULL
    };

    g_builtin_nodes_info[2] = (umugu_node_info) {
        .name = {.str = "Mixer"},
        .size_bytes = um__mixer_size,
        .var_count = um__mixer_var_count,
        .getfn = um__mixer_getfn,
        .vars = um__mixer_vars,
        .plug_handle = NULL
    };

    g_builtin_nodes_info[3] = (umugu_node_info) {
        .name = {.str = "Amplitude"},
        .size_bytes = um__amplitude_size,
        .var_count = um__amplitude_var_count,
        .getfn = um__amplitude_getfn,
        .vars = um__amplitude_vars,
        .plug_handle = NULL
    };

    g_builtin_nodes_info[4] = (umugu_node_info) {
        .name = {.str = "Limiter"},
        .size_bytes = um__limiter_size,
        .var_count = um__limiter_var_count,
        .getfn = um__limiter_getfn,
        .vars = um__limiter_vars,
        .plug_handle = NULL
    };

    return UMUGU_SUCCESS;
}

static inline int um__name_equals(const umugu_name *a, const umugu_name *b)
{
    return !memcmp(a, b, UMUGU_NAME_LEN);
}

static inline const umugu_node_info *
um__node_info_builtin_find(const umugu_name *name) {
    for (int i = 0; i < UM__BUILTIN_NODES_COUNT; ++i) {
        if (!strncmp(g_builtin_nodes_info[i].name.str, name->str,
                     UMUGU_NAME_LEN)) {
            return g_builtin_nodes_info + i;
        }
    }
    return NULL;
}

const umugu_node_info * umugu_node_info_find(const umugu_name *name)
{
    for (int i = 0; i < g_ctx->nodes_info_next; ++i) {
        if (um__name_equals(&g_ctx->nodes_info[i].name, name)) {
            return g_ctx->nodes_info + i;
        }
    }

    return NULL;
}

const umugu_node_info *
umugu_node_info_load(const umugu_name *name)
{
    /* Check if the node info is already loaded. */
    const umugu_node_info *found = umugu_node_info_find(name);
    if (found) {
        return found;
    }

    /* Look for it in the built-in nodes. */
    const umugu_node_info *bi_info = um__node_info_builtin_find(name);
    if (bi_info) {
        /* Add the node info to the current context. */
        g_ctx->assert(g_ctx->nodes_info_next < UMUGU_DEFAULT_NODE_INFO_CAPACITY);
        umugu_node_info *info = &g_ctx->nodes_info[g_ctx->nodes_info_next++];
        memcpy(info, bi_info, sizeof(umugu_node_info));
        return info;
    }

    /* Check if there's a plug with that name and load it. */
    int ret = umugu_plug(name);
    if (ret >= 0) {
        return g_ctx->nodes_info + ret;
    }

    /* Node info not found. */
    g_ctx->assert(ret == UMUGU_ERR_PLUG);
    g_ctx->log("Node type %s not found.\n", name->str);
    return NULL;
}

void umugu_set_context(umugu_ctx *new_ctx)
{
    g_ctx = new_ctx;
}

umugu_ctx *umugu_get_context(void)
{
    return g_ctx;
}

int umugu_save_pipeline_bin(const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f) {
        g_ctx->log("Could not open the file %s. Aborting pipeline save.", filename);
        return UMUGU_ERR_FILE;
    }

    struct {
        int32_t head;
        int32_t version;
        int64_t size;
    } header = { .head = UMUGU_HEAD_CODE,
                 .version = UMUGU_GET_VERSION,
                 .size = g_ctx->pipeline.size_bytes };
    fwrite(&header, sizeof(header), 1, f);
    fwrite(g_ctx->pipeline.root, header.size, 1, f);
    int32_t tail = UMUGU_TAIL_CODE;
    fwrite(&tail, sizeof(tail), 1, f);
    fclose(f);

    return UMUGU_SUCCESS;
}

int umugu_load_pipeline_bin(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        g_ctx->log("Could not open the file %s. Aborting pipeline load.\n", filename);
        return UMUGU_ERR_FILE;
    }

    struct {
        int32_t head;
        int32_t version;
        int64_t size;
    } header;

    fread(&header, sizeof(header), 1, f);
    if (header.head != UMUGU_HEAD_CODE) {
        g_ctx->log("Binary file %s header code does not match.\n", filename);
        fclose(f);
        return UMUGU_ERR_FILE;
    }
    /* TODO(Err): Check if version is supported. */

    /* TODO? Remove this?? */
    if (g_ctx->pipeline.root) {
        g_ctx->assert(g_ctx->pipeline.size_bytes && "Zero size but non-NULL?");
        // hmmmm... free?
        g_ctx->free(g_ctx->pipeline.root);
    }

    g_ctx->pipeline.size_bytes = header.size;
    g_ctx->pipeline.root = g_ctx->alloc(header.size);

    if (!g_ctx->pipeline.root) {
        g_ctx->log("Allocation failed. Aborting pipeline load.\n");
        fclose(f);
        return UMUGU_ERR_MEM;
    }

    fread(g_ctx->pipeline.root, header.size, 1, f);
    int32_t tail;
    fread(&tail, sizeof(tail), 1, f);
    g_ctx->assert(tail == UMUGU_TAIL_CODE);
    fclose(f);

    /* Pipeline inits */
    umugu_node *it = g_ctx->pipeline.root;
    umugu_node_call(UMUGU_FN_INIT, &it, NULL);
    return UMUGU_SUCCESS;
}

int umugu_node_call(umugu_fn fn, umugu_node **node, umugu_signal *out)
{
    const umugu_name *name = (umugu_name*)*node;
    const umugu_node_info *info = umugu_node_info_find(name);
    if (!info) {
        info = umugu_node_info_load(name);
        if (!info) {
            g_ctx->log("Node type %s not found.\n", name->str);
            return UMUGU_ERR;
        }
    }
    info->getfn(fn)(node, out);
    return UMUGU_SUCCESS;
}

int umugu_plug(const umugu_name *name)
{
    char buf[1024];
    snprintf(buf, 1024, "lib%s.so", name->str);
    
    g_ctx->assert(g_ctx->nodes_info_next < UMUGU_DEFAULT_NODE_INFO_CAPACITY);
    void *hnd = dlopen(buf, RTLD_NOW);
    if (!hnd) {
        g_ctx->log("Can't load plug: dlopen(%s) failed.", buf);
        return UMUGU_ERR_PLUG;
    }

    umugu_node_info *info = &g_ctx->nodes_info[g_ctx->nodes_info_next++];
    strncpy(info->name.str, name->str, UMUGU_NAME_LEN);
    *(void**)&info->getfn = dlsym(hnd, "getfn");
    info->size_bytes = *(size_t*)dlsym(hnd, "size");
    info->vars = *(umugu_var_info**)dlsym(hnd, "vars");
    info->var_count = *(int32_t*)dlsym(hnd, "var_count");
    info->plug_handle = hnd;
    return g_ctx->nodes_info_next - 1;
}

void umugu_unplug(const umugu_name *name)
{
    umugu_node_info *info = (void*)umugu_node_info_find(name);
    if (info && info->plug_handle) {
        dlclose(info->plug_handle);
        umugu_node_info *last = &g_ctx->nodes_info[--g_ctx->nodes_info_next];
        if (info != last) {
            memcpy(info, &g_ctx->nodes_info[g_ctx->nodes_info_next], sizeof(*info));
        }
        memset(last, 0, sizeof(*last));
    }
}

int umugu_init(void)
{
    if (g_ctx->audio_output.open) {
        g_ctx->log(
"Umugu has already been initialized and the audio backend is loaded.\n"
"Ignoring this umugu_init() call...\n");
        return UMUGU_NOOP;
    }

    um__node_info_builtin_load();

    if (um__backend_init() == UMUGU_SUCCESS) {
        g_ctx->audio_output.open = 1;
    }

    return UMUGU_SUCCESS;
}

int umugu_start_stream(void)
{
    if (g_ctx->audio_output.running) {
        g_ctx->log(
"The output stream is already running.\n"
"Ignoring this umugu_start_stream() call.\n");
        return UMUGU_NOOP;
    }
    
    int ret = um__backend_start_stream();
    if (ret == UMUGU_SUCCESS) {
        g_ctx->audio_output.running = 1;
        return UMUGU_SUCCESS;
    }

    g_ctx->log("Unable to start the stream. Error %d.\n", ret);
    return UMUGU_ERR_STREAM;
}

int umugu_stop_stream(void)
{
    if (!g_ctx->audio_output.running) {
        g_ctx->log(
"Trying to stop a stopped stream.\n"
"Ignoring this umugu_stop_stream() call.\n");
        return UMUGU_NOOP;
    }
    
    int ret = um__backend_stop_stream();
    if (ret == UMUGU_SUCCESS) {
        g_ctx->audio_output.running = 0;
        return UMUGU_SUCCESS;
    }

    g_ctx->log("Unable to stop the stream. Error %d.\n", ret);
    return UMUGU_ERR_STREAM;
}

int umugu_close(void)
{
    /* This will abort the output stream if not stopped.
     * No point in checking. */
    g_ctx->audio_output.running = 0;
    g_ctx->audio_output.open = 0;
    return um__backend_close();
}
