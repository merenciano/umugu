#ifndef __UMUGU_DEBUG_H__
#define __UMUGU_DEBUG_H__

#include "umugu/umugu.h"

#ifndef UMUGU_DEBUG

static int
umugu_node_print(umugu_node *node)
{
    return UMUGU_NOOP;
}

static int
umugu_pipeline_print(void)
{
    return UMUGU_NOOP;
}

static int
umugu_mem_arena_print(void)
{
    return UMUGU_NOOP;
}

static int
umugu_out_signal_print(void)
{
    return UMUGU_NOOP;
}

#else

static inline void
um__var_print(umugu_ctx *ctx, const umugu_attrib_info *vi, void *node)
{
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

static inline int
umugu_node_print(umugu_ctx *ctx, umugu_node *node)
{
    int (*log)(const char *fmt, ...) = ctx->io.log;
    const umugu_node_type_info *info = &ctx->nodes_info[node->info_idx];
    log("Name: %s.\n", info->name.str);
    log("In pipe: %d.\n", node->prev_node);
    log("Variables:\n");
    for (int i = 0; i < info->attrib_count; ++i) {
        um__var_print(ctx, info->attribs + i, node);
    }

    return UMUGU_SUCCESS;
}

static inline int
umugu_pipeline_print(umugu_ctx *ctx)
{
    int (*log)(const char *fmt, ...) = ctx->io.log;
    log("\n # PIPELINE # \n");
    log("Node count: %d.\n", ctx->pipeline.node_count);
    log("-------------------------------------------\n");
    for (int i = 0; i < ctx->pipeline.node_count; ++i) {
        log("Node [%d] \n", i);
        umugu_node_print(ctx, ctx->pipeline.nodes[i]);
        log("-------------------------------------------\n");
    }
    return UMUGU_SUCCESS;
}

static inline int
umugu_mem_arena_print(umugu_ctx *ctx)
{
    int (*log)(const char *fmt, ...) = ctx->io.log;

    if (ctx->arena_tail < ctx->arena_pers_end) {
        ctx->arena_tail = ctx->arena_pers_end;
    }

    log("\n # MEMORY ARENA # \n");
    log("The values are offsets (bytes) in the arena buffer.\n");
    log("Persistent: %ld\n", ctx->arena_pers_end - ctx->arena_head);
    log("Temporal: %ld (%ld from pers)\n", ctx->arena_tail - ctx->arena_head,
        ctx->arena_tail - ctx->arena_pers_end);
    log("Capacity: %d\n", ctx->arena_capacity);
    log("Data after temporal ptr:");
    for (uint64_t *it = (void *)ctx->arena_tail;
         it < (uint64_t *)(ctx->arena_head + ctx->arena_capacity) &&
         it < (uint64_t *)ctx->arena_tail + 16;
         ++it) {
        if ((it - (uint64_t *)ctx->arena_tail) % 8 == 0) {
            log("\n\t");
        }
        log("0x%X ", *it);
    }
    log("\n----------------------------------\n");
    return UMUGU_SUCCESS;
}

static inline const char *
um__type_name(int type)
{
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

static inline void
um__signal_print(umugu_ctx *ctx, umugu_signal *sig)
{
    int (*log)(const char *fmt, ...) = ctx->io.log;
    const char *intr = sig->interleaved_channels ? "true" : "false";

    log("Type: %s (%d).\n", um__type_name(sig->format), sig->format);
    log("Sample rate: %d.\n", sig->sample_rate);
    log("Channels: %d.\n", sig->samples.channel_count);
    log("Interleaved: %s.\n", intr);
}

static inline int
umugu_out_signal_print(umugu_ctx *ctx)
{
    int (*log)(const char *fmt, ...) = ctx->io.log;
    log("\n # OUTPUT AUDIO # \n");
    um__signal_print(ctx, &ctx->io.out_audio);
    log("----------------------------------\n");
    return UMUGU_SUCCESS;
}

#endif /* UMUGU_DEBUG */

#endif /* __UMUGU_DEBUG_H__ */
