#include "debugu.h"

#include "umugu/umugu.h"

static inline void um__var_print(const umugu_var_info *vi, void *node) {
    umugu_ctx *ctx = umugu_get_context();
    void *var = (char *)node + vi->offset_bytes;
    switch (vi->type) {
    case UMUGU_TYPE_FLOAT:
        ctx->io.log("%s: %f.\n", vi->name.str, *(float *)var);
        break;
    case UMUGU_TYPE_INT8:
        ctx->io.log("%s: %d.\n", vi->name.str, *(int8_t *)var);
        break;
    case UMUGU_TYPE_INT16:
        ctx->io.log("%s: %d.\n", vi->name.str, *(int16_t *)var);
        break;
    case UMUGU_TYPE_INT32:
        ctx->io.log("%s: %d.\n", vi->name.str, *(int32_t *)var);
        break;
    case UMUGU_TYPE_INT64:
        ctx->io.log("%s: %ld.\n", vi->name.str, *(int64_t *)var);
        break;
    case UMUGU_TYPE_UINT8:
        ctx->io.log("%s: %u.\n", vi->name.str, *(uint8_t *)var);
        break;
    case UMUGU_TYPE_TEXT:
        ctx->io.log("%s: %s.\n", vi->name.str, (const char *)var);
        break;
    case UMUGU_TYPE_BOOL:
        ctx->io.log("%s: %s.\n", vi->name.str, *(int *)var ? "true" : "false");
        break;
    default:
        ctx->io.log("%s: Unimplemented type %d.\n", vi->type);
        break;
    }
}

int umugu_node_print(umugu_node *node) {
    umugu_ctx *ctx = umugu_get_context();
    int (*log)(const char *fmt, ...) = ctx->io.log;
    const umugu_node_info *info = &ctx->nodes_info[node->info_idx];
    log("Name: %s.\n", info->name.str);
    log("In pipe: %d.\n", node->in_pipe_node);

    log("Variables:\n");
    for (int i = 0; i < info->var_count; ++i) {
        um__var_print(info->vars + i, node);
    }

    return UMUGU_SUCCESS;
}

int umugu_pipeline_print(void) {
    umugu_ctx *ctx = umugu_get_context();
    int (*log)(const char *fmt, ...) = ctx->io.log;
    log("\n # PIPELINE # \n");
    log("Node count: %d.\n", ctx->pipeline.node_count);
    log("-------------------------------------------\n");
    for (int i = 0; i < ctx->pipeline.node_count; ++i) {
        log("Node [%d] \n", i);
        umugu_node_print(ctx->pipeline.nodes[i]);
        log("-------------------------------------------\n");
    }
    return UMUGU_SUCCESS;
}

int umugu_mem_arena_print(void) {
    umugu_ctx *ctx = umugu_get_context();
    int (*log)(const char *fmt, ...) = ctx->io.log;

    if (ctx->arena.temp_next < ctx->arena.pers_next) {
        ctx->arena.temp_next = ctx->arena.pers_next;
    }
    log("\n # MEMORY ARENA # \n");
    log("The values are offsets (bytes) in the arena buffer.\n");
    log("Persistent: %ld\n", ctx->arena.pers_next - ctx->arena.buffer);
    log("Temporal: %ld (%ld from pers)\n",
        ctx->arena.temp_next - ctx->arena.buffer,
        ctx->arena.temp_next - ctx->arena.pers_next);
    log("Capacity: %d\n", ctx->arena.capacity);

    log("Data after temporal ptr:");
    for (uint64_t *it = (void *)ctx->arena.temp_next;
         it < (uint64_t *)(ctx->arena.buffer + ctx->arena.capacity) &&
         it < (uint64_t *)ctx->arena.temp_next + 16;
         ++it) {
        if ((it - (uint64_t *)ctx->arena.temp_next) % 8 == 0) {
            log("\n\t");
        }
        log("0x%X ", *it);
    }
    log("\n----------------------------------\n");
    return UMUGU_SUCCESS;
}

static inline const char *um__type_name(int type) {
    switch (type) {
    case UMUGU_TYPE_FLOAT:
        return "float";
    case UMUGU_TYPE_UINT8:
        return "uint8";
    case UMUGU_TYPE_INT8:
        return "int8";
    case UMUGU_TYPE_INT16:
        return "int16";
    case UMUGU_TYPE_INT32:
        return "int32";
    default:
        return "Unknown";
    }
}

static inline void um__generic_signal_print(umugu_generic_signal *sig) {
    int (*log)(const char *fmt, ...) = umugu_get_context()->io.log;
    log("Enabled: %s.\n", sig->flags & UMUGU_SIGNAL_ENABLED ? "true" : "false");
    log("Type: %s (%d).\n", um__type_name(sig->type), sig->type);
    log("Sample rate: %d.\n", sig->rate);
    log("Channels: %d.\n", sig->channels);
    log("Interleaved: %s.\n",
        sig->flags & UMUGU_SIGNAL_INTERLEAVED ? "true" : "false");
}

int umugu_in_signal_print(void) {
    umugu_ctx *ctx = umugu_get_context();
    int (*log)(const char *fmt, ...) = ctx->io.log;
    log("\n # INPUT AUDIO # \n");
    um__generic_signal_print(&ctx->io.in_audio);
    log("----------------------------------\n");
    return UMUGU_SUCCESS;
}

int umugu_out_signal_print(void) {
    umugu_ctx *ctx = umugu_get_context();
    int (*log)(const char *fmt, ...) = ctx->io.log;
    log("\n # OUTPUT AUDIO # \n");
    um__generic_signal_print(&ctx->io.out_audio);
    log("----------------------------------\n");
    return UMUGU_SUCCESS;
}
