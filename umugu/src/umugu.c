#include "umugu.h"

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

    {.name = {.str = "ControlMidi"},
     .size_bytes = um__ctrlmidi_size,
     .var_count = um__ctrlmidi_var_count,
     .getfn = um__ctrlmidi_getfn,
     .vars = um__ctrlmidi_vars,
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

static inline int um__node_info_builtin_load(void) {
#if 0
    g_builtin_nodes_info[0] =
        (umugu_node_info){.name = {.str = "Oscilloscope"},
                          .size_bytes = um__oscope_size,
                          .var_count = um__oscope_var_count,
                          .getfn = um__oscope_getfn,
                          .vars = um__oscope_vars,
                          .plug_handle = NULL};

    g_builtin_nodes_info[1] =
        (umugu_node_info){.name = {.str = "WavFilePlayer"},
                          .size_bytes = um__wavplayer_size,
                          .var_count = um__wavplayer_var_count,
                          .getfn = um__wavplayer_getfn,
                          .vars = um__wavplayer_vars,
                          .plug_handle = NULL};

    g_builtin_nodes_info[2] =
        (umugu_node_info){.name = {.str = "Mixer"},
                          .size_bytes = um__mixer_size,
                          .var_count = um__mixer_var_count,
                          .getfn = um__mixer_getfn,
                          .vars = um__mixer_vars,
                          .plug_handle = NULL};

    g_builtin_nodes_info[3] =
        (umugu_node_info){.name = {.str = "Amplitude"},
                          .size_bytes = um__amplitude_size,
                          .var_count = um__amplitude_var_count,
                          .getfn = um__amplitude_getfn,
                          .vars = um__amplitude_vars,
                          .plug_handle = NULL};

    g_builtin_nodes_info[4] =
        (umugu_node_info){.name = {.str = "Limiter"},
                          .size_bytes = um__limiter_size,
                          .var_count = um__limiter_var_count,
                          .getfn = um__limiter_getfn,
                          .vars = um__limiter_vars,
                          .plug_handle = NULL};

    g_builtin_nodes_info[5] =
        (umugu_node_info){.name = {.str = "ControlMidi"},
                          .size_bytes = um__ctrlmidi_size,
                          .var_count = um__ctrlmidi_var_count,
                          .getfn = um__ctrlmidi_getfn,
                          .vars = um__ctrlmidi_vars,
                          .plug_handle = NULL};

    g_builtin_nodes_info[6] =
        (umugu_node_info){.name = {.str = "Piano"},
                          .size_bytes = um__piano_size,
                          .var_count = um__piano_var_count,
                          .getfn = um__piano_getfn,
                          .vars = um__piano_vars,
                          .plug_handle = NULL};

    g_builtin_nodes_info[7] =
        (umugu_node_info){.name = {.str = "Output"},
                          .size_bytes = um__output_size,
                          .var_count = um__output_var_count,
                          .getfn = um__output_getfn,
                          .vars = um__output_vars,
                          .plug_handle = NULL};

#endif
    return UMUGU_SUCCESS;
}

static inline int um__name_equals(const umugu_name *a, const umugu_name *b) {
    return !memcmp(a, b, UMUGU_NAME_LEN);
}

static inline const umugu_node_info *
um__node_info_builtin_find(const umugu_name *name) {
    for (int i = 0; i < um__builtin_nodes_info_count; ++i) {
        if (!strncmp(g_builtin_nodes_info[i].name.str, name->str,
                     UMUGU_NAME_LEN)) {
            return g_builtin_nodes_info + i;
        }
    }
    return NULL;
}

const umugu_node_info *umugu_node_info_find(const umugu_name *name) {
    for (int i = 0; i < g_ctx->nodes_info_next; ++i) {
        if (um__name_equals(&g_ctx->nodes_info[i].name, name)) {
            return g_ctx->nodes_info + i;
        }
    }

    return NULL;
}

const umugu_node_info *umugu_node_info_load(const umugu_name *name) {
    /* Check if the node info is already loaded. */
    const umugu_node_info *found = umugu_node_info_find(name);
    if (found) {
        return found;
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

const umugu_var_info *umugu_var_info_get(umugu_name node, int var_idx) {
    assert(var_idx >= 0 && "Var index can not be negative.");
    assert(var_idx < 128 && "Delete this assert if var_idx is correct.");
    const umugu_node_info *info = umugu_node_info_find(&node);
    if (!info) {
        return NULL;
    }

    return &info->vars[var_idx];
}

int umugu_newframe(void) {
    for (int i = 0; i < g_ctx->pipeline.node_count; ++i) {
        g_ctx->pipeline.nodes[i]->pipe_out_ready = 0;
    }

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
                          err, i, g_ctx->nodes_info[node->node_info_idx].name);
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

umugu_frame *umugu_get_temp_signal(umugu_signal *s) {
    int frame_count = g_ctx->io.out_audio_signal.count;
    assert(s && (frame_count > 0));
    s->frames = umugu_alloc_temp(frame_count * sizeof(umugu_frame));
    s->count = frame_count;
    s->capacity = frame_count;
    return s->frames;
}

int umugu_node_dispatch(umugu_node *node, umugu_fn fn) {
    assert(node->node_info_idx >= 0 && "Node info index can not be negative.");
    assert(g_ctx->nodes_info_next > node->node_info_idx &&
           "Node info not loaded.");
    return g_ctx->nodes_info[node->node_info_idx].getfn(fn)(node);
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

void umugu_unplug(const umugu_name *name) {
    umugu_node_info *info = (void *)umugu_node_info_find(name);
    if (info && info->plug_handle) {
        dlclose(info->plug_handle);
        umugu_node_info *last = &g_ctx->nodes_info[--g_ctx->nodes_info_next];
        if (info != last) {
            memcpy(info, &g_ctx->nodes_info[g_ctx->nodes_info_next],
                   sizeof(*info));
        }
        memset(last, 0, sizeof(*last));
    }
}

int umugu_init(void) { return um__node_info_builtin_load(); }

int umugu_close(void) { return UMUGU_SUCCESS; }
