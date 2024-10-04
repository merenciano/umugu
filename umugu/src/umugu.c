#include "umugu.h"
#include "umugu_internal.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#define UMUGU_CONFIG_NODE_INFO_CAPACITY 64

#define UMUGU_HEAD_CODE 0x00454549
#define UMUGU_TAIL_CODE 0x00555541

#define UMUGU_GET_VERSION \
    ((UMUGU_VERSION_MAJOR << 20) | \
     (UMUGU_VERSION_MINOR << 10) | \
      UMUGU_VERSION_PATCH)

typedef struct um__nodeinfo {
    umugu_name name;
    umugu_node_fn (*getfn)(umugu_code fn);
    size_t size_bytes;
    void *handle; /* dynamic lib handle */
} um__nodeinfo;

typedef struct um__nodeinfo_arr {
    const int max;
    int next;
    um__nodeinfo info[UMUGU_CONFIG_NODE_INFO_CAPACITY];
} um__nodeinfo_arr;

static um__nodeinfo_arr nodes = { .max = UMUGU_CONFIG_NODE_INFO_CAPACITY, .next = 0 };

/* The default ctx relies in being zero-initialized for being static. */
static umugu_ctx g_default_ctx = { .sample_rate = UMUGU_SAMPLE_RATE, .audio_src_sample_capacity = 1024 };
static umugu_ctx *g_ctx = &g_default_ctx;

static inline um__nodeinfo *um__find_info(umugu_name *name)
{
    for (int i = 0; i < nodes.next; ++i) {
        if (!strncmp(nodes.info[i].name.str, name->str, UMUGU_NAME_LEN)) {
            return nodes.info + i;
        }
    }
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

void umugu_save_pipeline_bin(const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f) {
        g_ctx->err = UMUGU_ERR_FILE;
        snprintf(g_ctx->err_msg, UMUGU_ERR_MSG_LEN,
            "Could not open the file %s. Aborting pipeline save.", filename);
        return;
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
}

void umugu_load_pipeline_bin(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        g_ctx->err = UMUGU_ERR_FILE;
        snprintf(g_ctx->err_msg, UMUGU_ERR_MSG_LEN,
            "Could not open the file %s. Aborting pipeline load.", filename);
        return;
    }

    struct {
        int32_t head;
        int32_t version;
        int64_t size;
    } header;

    fread(&header, sizeof(header), 1, f);
    if (header.head != UMUGU_HEAD_CODE) {
        g_ctx->err = UMUGU_ERR_FILE;
        snprintf(g_ctx->err_msg, UMUGU_ERR_MSG_LEN,
            "Binary file %s header code does not match.", filename);
        fclose(f);
        return;
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
        g_ctx->err = UMUGU_ERR_MEM;
        strcpy(g_ctx->err_msg, "Allocation failed. Aborting pipeline load.");
        fclose(f);
        return;
    }

    fread(g_ctx->pipeline.root, header.size, 1, f);
    int32_t tail;
    fread(&tail, sizeof(tail), 1, f);
    g_ctx->assert(tail == UMUGU_TAIL_CODE);
    fclose(f);

    /* Pipeline inits */
    umugu_node *it = g_ctx->pipeline.root;
    umugu_node_call(UMUGU_FN_INIT, &it, NULL);
}

void umugu_init(void)
{
    /* Add built-in nodes */
    {
        /* Oscilloscope */
        g_ctx->assert(nodes.next < nodes.max);
        um__nodeinfo *info = &nodes.info[nodes.next++];
        *info = (struct um__nodeinfo) {
            .name = {.str = "Oscilloscope"},
            .getfn = um__oscope_getfn,
            .size_bytes = sizeof(um__oscope),
            .handle = NULL };
    }

    {
        /* .wav file player */
        g_ctx->assert(nodes.next < nodes.max);
        um__nodeinfo *info = &nodes.info[nodes.next++];
        *info = (struct um__nodeinfo) {
            .name = {.str = "WavFilePlayer"},
            .getfn = um__wavplayer_getfn,
            .size_bytes = sizeof(um__wav_player),
            .handle = NULL };
    }

    {
        /* Mixer */
        g_ctx->assert(nodes.next < nodes.max);
        um__nodeinfo *info = &nodes.info[nodes.next++];
        *info = (struct um__nodeinfo) {
            .name = {.str = "Mixer"},
            .getfn = um__mixer_getfn,
            .size_bytes = sizeof(um__mixer),
            .handle = NULL };
    }

    {
        /* Amplitude */
        g_ctx->assert(nodes.next < nodes.max);
        um__nodeinfo *info = &nodes.info[nodes.next++];
        *info = (struct um__nodeinfo) {
            .name = {.str = "Amplitude"},
            .getfn = um__amplitude_getfn,
            .size_bytes = sizeof(um__amplitude),
            .handle = NULL };
    }

    {
        /* Limiter */
        g_ctx->assert(nodes.next < nodes.max);
        um__nodeinfo *info = &nodes.info[nodes.next++];
        *info = (struct um__nodeinfo) {
            .name = {.str = "Limiter"},
            .getfn = um__limiter_getfn,
            .size_bytes = sizeof(um__limiter),
            .handle = NULL };
    }

    umugu_init_backend();
}

void umugu_node_call(umugu_code fn, umugu_node **node, umugu_signal *out)
{
    um__nodeinfo *info = um__find_info((umugu_name*)*node);
    if (!info) {
        int ret = umugu_plug(&((*node)->name));
        if (ret == UMUGU_INVALID) {
            g_ctx->err = UMUGU_ERR;
            snprintf(g_ctx->err_msg, UMUGU_ERR_MSG_LEN, "Node type %s not found.", (*node)->name.str);
            return;
        }
        info = &nodes.info[ret];
    }
    info->getfn(fn)(node, out);
}

int umugu_plug(umugu_name *name)
{
    char buf[1024];
    snprintf(buf, 1024, "lib%s.so", name->str);
    
    g_ctx->assert(nodes.next < nodes.max);
    void *hnd = dlopen(buf, RTLD_NOW);
    if (!hnd) {
        g_ctx->err = UMUGU_ERR_PLUG;
        snprintf(g_ctx->err_msg, UMUGU_ERR_MSG_LEN, "Can't load plug: dlopen(%s) failed.", buf);
        return UMUGU_INVALID;
    }

    um__nodeinfo *info = &nodes.info[nodes.next++];
    strncpy(info->name.str, name->str, UMUGU_NAME_LEN);
    info->getfn = (umugu_node_fn (*)(umugu_code))dlsym(hnd, "getfn");
    info->size_bytes = *(size_t*)dlsym(hnd, "size");
    info->handle = hnd;
    return nodes.next - 1;
}

void umugu_unplug(umugu_name *name)
{
    um__nodeinfo *info = um__find_info(name);
    if (info) {
        dlclose(info->handle);
        um__nodeinfo *last = &nodes.info[--nodes.next];
        if (info != last) {
            memcpy(info, &nodes.info[nodes.next], sizeof(*info));
        }
        memset(last, 0, sizeof(*last));
    }
}

void umugu_close(void)
{
    umugu_close_backend();
}
