#include "umugu.h"

#include "controller.h"
#include "umutils.h"

#define UMUGU_BUILTIN_NODES_DEFINITIONS
#include "builtin_nodes.h"

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UM_PTR(PTR, OFFSET_BYTES)                                              \
    ((void *)((char *)((void *)(PTR)) + (OFFSET_BYTES)))

static umugu_ctx g_default_ctx; /* Relying on zero-init from static memory. */
static umugu_ctx *g_ctx = &g_default_ctx;

const umugu_node_info g_builtin_nodes_info[] = {
    {.name = {.str = "Oscilloscope"},
     .size_bytes = um__oscope_size,
     .var_count = um__oscope_var_count,
     .getfn = um__oscope_getfn,
     .vars = um__oscope_vars,
     .plug_handle = NULL},

    {.name = {.str = "WavFilePlayer"},
     .size_bytes = um__wavplayer_size,
     .var_count = um__wavplayer_var_count,
     .getfn = um__wavplayer_getfn,
     .vars = um__wavplayer_vars,
     .plug_handle = NULL},

    {.name = {.str = "Mixer"},
     .size_bytes = um__mixer_size,
     .var_count = um__mixer_var_count,
     .getfn = um__mixer_getfn,
     .vars = um__mixer_vars,
     .plug_handle = NULL},

    {.name = {.str = "Amplitude"},
     .size_bytes = um__amplitude_size,
     .var_count = um__amplitude_var_count,
     .getfn = um__amplitude_getfn,
     .vars = um__amplitude_vars,
     .plug_handle = NULL},

    {.name = {.str = "Limiter"},
     .size_bytes = um__limiter_size,
     .var_count = um__limiter_var_count,
     .getfn = um__limiter_getfn,
     .vars = um__limiter_vars,
     .plug_handle = NULL},

    {.name = {.str = "Piano"},
     .size_bytes = um__piano_size,
     .var_count = um__piano_var_count,
     .getfn = um__piano_getfn,
     .vars = um__piano_vars,
     .plug_handle = NULL},

    {.name = {.str = "Output"},
     .size_bytes = um__output_size,
     .var_count = um__output_var_count,
     .getfn = um__output_getfn,
     .vars = um__output_vars,
     .plug_handle = NULL},
};

static const size_t um__builtin_nodes_info_count =
    UM__ARRAY_COUNT(g_builtin_nodes_info);

static inline int um__name_equals(const umugu_name *a, const umugu_name *b) {
    return !memcmp(a, b, UMUGU_NAME_LEN);
}

static inline const umugu_node_info *
um__node_info_builtin_find(const umugu_name *name) {
    for (size_t i = 0; i < um__builtin_nodes_info_count; ++i) {
        if (!strncmp(g_builtin_nodes_info[i].name.str, name->str,
                     UMUGU_NAME_LEN)) {
            return g_builtin_nodes_info + i;
        }
    }
    return NULL;
}

const umugu_node_info *umugu_node_info_load(const umugu_name *name) {
    /* Check if the node info is already loaded. */
    for (int i = 0; i < g_ctx->nodes_info_next; ++i) {
        if (um__name_equals(&g_ctx->nodes_info[i].name, name)) {
            return g_ctx->nodes_info + i;
        }
    }

    /* Look for it in the built-in nodes. */
    const umugu_node_info *bi_info = um__node_info_builtin_find(name);
    if (bi_info) {
        /* Add the node info to the current context. */
        assert(g_ctx->nodes_info_next < UMUGU_DEFAULT_NODE_INFO_CAPACITY);
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
    assert(ret == UMUGU_ERR_PLUG);
    g_ctx->io.log("Node type %s not found.\n", name->str);
    return NULL;
}

int umugu_init(void) {
    umu_nanosec init_time = umu_time_now();
    umugu_ctrl_init();
    g_ctx->init_time_ns = umu_time_elapsed(init_time);
    return UMUGU_SUCCESS;
}

int umugu_newframe(void) {
    for (int i = 0; i < g_ctx->pipeline.node_count; ++i) {
        g_ctx->pipeline.nodes[i]->out_pipe.samples = NULL;
    }

    umugu_ctrl_update();

    return UMUGU_SUCCESS;
}

int umugu_produce_signal(void) {
    if (!g_ctx->pipeline.node_count) {
        return UMUGU_NOOP;
    }

    for (int i = 0; i < g_ctx->pipeline.node_count; ++i) {
        umugu_node *node = g_ctx->pipeline.nodes[i];
        int err = umugu_node_dispatch(node, UMUGU_FN_PROCESS);
        if (err < UMUGU_SUCCESS) {
            g_ctx->io.log("Error (%d) processing node:\n"
                          "\tIndex: %d.\n\tName: %s\n",
                          err, i, g_ctx->nodes_info[node->info_idx].name);
        }
    }

    return UMUGU_SUCCESS;
}

void umugu_set_context(umugu_ctx *new_ctx) { g_ctx = new_ctx; }

umugu_ctx *umugu_get_context(void) { return g_ctx; }

int umugu_set_arena(void *buffer, size_t bytes) {
    g_ctx->arena.buffer = buffer;
    g_ctx->arena.capacity = bytes;
    g_ctx->arena.pers_next = buffer;
    g_ctx->arena.temp_next = buffer;

    return UMUGU_SUCCESS;
}

void *umugu_alloc_pers(size_t bytes) {
    umugu_mem_arena *mem = &g_ctx->arena;
    uint8_t *ret = mem->pers_next;
    if ((ret + bytes) > (mem->buffer + mem->capacity)) {
        umugu_get_context()->io.log("Fatal error: Alloc failed. Not enough "
                                    "space left in the arena.\n");
        return NULL;
    }
    mem->pers_next += bytes;
    assert(mem->pers_next < (mem->buffer + mem->capacity));
    return ret;
}

void *umugu_alloc_temp(size_t bytes) {
    umugu_mem_arena *mem = &g_ctx->arena;
    uint8_t *ret =
        mem->temp_next > mem->pers_next ? mem->temp_next : mem->pers_next;
    if ((ret + bytes) > (mem->buffer + mem->capacity)) {
        /* Start again: temp memory is used as a circ buffer. */
        ret = mem->pers_next;
        if ((ret + bytes) > (mem->buffer + mem->capacity)) {
            umugu_get_context()->io.log("Fatal error: Alloc failed. Not enough "
                                        "space left in the arena.\n");
            return NULL;
        }
    }
    mem->temp_next = ret + bytes;
    return ret;
}

umugu_sample *umugu_alloc_signal(umugu_signal *s) {
    assert(s);
    assert(s->channels > 0 && s->channels < 32 &&
           "Invalid number of channels.");
    s->count = g_ctx->io.out_audio.count;
    assert(s->count > 0);
    s->samples =
        umugu_alloc_temp(s->count * sizeof(umugu_sample) * s->channels);
    return s->samples;
}

void *umugu_alloc_generic_signal(umugu_generic_signal *s) {
    assert(s);
    s->count = g_ctx->io.out_audio.count;
    assert(s->count > 0);
    s->sample_data =
        umugu_alloc_temp(s->count * umu_type_size(s->type) * s->channels);
    return s->sample_data;
}

int umugu_node_dispatch(umugu_node *node, umugu_fn fn) {
    assert(node->info_idx >= 0 && "Node info index can not be negative.");
    assert(g_ctx->nodes_info_next > node->info_idx && "Node info not loaded.");
    umugu_node_fn func = g_ctx->nodes_info[node->info_idx].getfn(fn);
    return func ? func(node) : UMUGU_ERR_NULL;
}

int umugu_plug(const umugu_name *name) {
    char buf[1024];
    snprintf(buf, 1024, "lib%s.so", name->str);

    assert(g_ctx->nodes_info_next < UMUGU_DEFAULT_NODE_INFO_CAPACITY);
    void *hnd = dlopen(buf, RTLD_NOW);
    if (!hnd) {
        g_ctx->io.log("Can't load plug: dlopen(%s) failed.", buf);
        return UMUGU_ERR_PLUG;
    }

    umugu_node_info *info = &g_ctx->nodes_info[g_ctx->nodes_info_next++];
    strncpy(info->name.str, name->str, UMUGU_NAME_LEN);
    *(void **)&info->getfn = dlsym(hnd, "getfn");
    info->size_bytes = *(size_t *)dlsym(hnd, "size");
    info->vars = *(umugu_var_info **)dlsym(hnd, "vars");
    info->var_count = *(int32_t *)dlsym(hnd, "var_count");
    info->plug_handle = hnd;
    return g_ctx->nodes_info_next - 1;
}

void umugu_unplug(umugu_node_info *info) {
    if (info && info->plug_handle) {
        dlclose(info->plug_handle);
        memset(info, 0, sizeof(*info));
    }
}
