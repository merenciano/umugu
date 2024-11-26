#ifndef __UMUGU_SERIALIZATION_H__
#define __UMUGU_SERIALIZATION_H__

#include "umugu/umugu.h"
#include "umugu_internal.h"

#include <stdint.h>
#include <stdio.h>

#define UMUGU_HEAD_CODE 0x00454549
#define UMUGU_FOOT_CODE 0x00555541

typedef struct {
    int32_t header;
    int32_t version;
    int32_t node_count;
} umugu_binary_header;

int
umugu_pipeline_export(umugu_ctx *ctx, const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f) {
        ctx->io.log("Error: fopen('wb') failed with filename %s\n", filename);
        return UMUGU_ERR_FILE;
    }

    umugu_binary_header h = {
        .header = UMUGU_HEAD_CODE,
        .version = UMUGU_VERSION,
        .node_count = ctx->pipeline.node_count};

    fwrite(&h, sizeof(h), 1, f);

    for (int i = 0; i < h.node_count; ++i) {
        umugu_node *n = ctx->pipeline.nodes[i];
        fwrite(&ctx->nodes_info[n->info_idx].name, sizeof(umugu_name), 1, f);
    }

    for (int i = 0; i < h.node_count; ++i) {
        umugu_node *n = ctx->pipeline.nodes[i];
        fwrite(n, ctx->nodes_info[n->info_idx].size_bytes, 1, f);
    }

    int32_t footer = UMUGU_FOOT_CODE;
    fwrite(&footer, 4, 1, f);
    fclose(f);
    return UMUGU_SUCCESS;
}

int
umugu_pipeline_import(umugu_ctx *ctx, const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        ctx->io.log("Error: fopen('rb') failed with filename %s\n", filename);
        return UMUGU_ERR_FILE;
    }

    umugu_binary_header h;

    fread(&h, sizeof(h), 1, f);

    if (h.header != UMUGU_HEAD_CODE) {
        ctx->io.log("Import pipeline error: Bad binary format.\n");
        fclose(f);
        return UMUGU_ERR_FILE;
    }

    if (h.version != UMUGU_VERSION) {
        ctx->io.log(
            "Imported file's version mismatch. The format may have changed.\n"
            "File: %d, Current: %d.\n",
            h.version, UMUGU_VERSION);
    }

    umugu_name names[h.node_count];
    fread(&names, sizeof(umugu_name), h.node_count, f);
    ctx->pipeline.node_count = h.node_count;

    size_t node_buffer_bytes = 0;

    for (int i = 0; i < h.node_count; ++i) {
        const umugu_node_type_info *ni = umugu_node_info_load(ctx, &names[i]);
        /* Storing the offsets because I don't have the node_buffer yet. */
        ctx->pipeline.nodes[i] = (void *)node_buffer_bytes;
        node_buffer_bytes += ni->size_bytes;
    }

    char *node_buffer = um_alloc_persistent(ctx, node_buffer_bytes);

    for (int i = 0; i < h.node_count; ++i) {
        /* Adding the buffer address to the previously cached offsets. */
        ctx->pipeline.nodes[i] = (void *)((size_t)ctx->pipeline.nodes[i] + node_buffer);
    }

    fread(node_buffer, node_buffer_bytes, 1, f);

    int footer;
    fread(&footer, 4, 1, f);
    if (footer != UMUGU_FOOT_CODE) {
        ctx->io.log("Import pipeline error: Bad binary format.\n");
        fclose(f);
        return UMUGU_ERR_FILE;
    }

    fclose(f);

    for (int i = 0; i < ctx->pipeline.node_count; ++i) {
        umugu_node_dispatch(ctx, ctx->pipeline.nodes[i], UMUGU_FN_INIT, UMUGU_NOFLAG);
    }
    return UMUGU_SUCCESS;
}

int
um_pipeline_generate(umugu_ctx *ctx, const umugu_name *node_names, int node_count)
{
    UMUGU_ASSERT(ctx);
    UMUGU_ASSERT(ctx->pipeline.node_count == 0);
    ctx->pipeline.node_count = node_count;

    int info_indices[64];
    size_t pipeline_size = 0;

    for (int i = 0; i < node_count; ++i) {
        const umugu_node_type_info *ni = umugu_node_info_load(ctx, &node_names[i]);
        if (!ni) {
            ctx->io.log("Could not load node %s\n", node_names[i].str);
            if (i == 0) {
                umugu_name fallback_generator = {.str = "Oscillator"};
                ni = umugu_node_info_load(ctx, &fallback_generator);
                if (!ni) {
                    ctx->io.fatal(
                        UMUGU_ERR,
                        "Failed to create both, the specified node and the fallback (Oscillator).",
                        __FILE__, __LINE__);
                    UMUGU_TRAP();
                }
            } else if (i == (node_count - 1)) {
                ctx->io.log("Ignoring it since it is the last.\n");
                --node_count;
                --ctx->pipeline.node_count;
                break;
            } else {
                umugu_name fallback_node = {.str = "Amplitude"};
                ni = umugu_node_info_load(ctx, &fallback_node);
                if (!ni) {
                    ctx->io.fatal(
                        UMUGU_ERR,
                        "Failed to create both, the specified node and the fallback (Amplitude).",
                        __FILE__, __LINE__);
                    UMUGU_TRAP();
                }
            }
        }

        UMUGU_ASSERT(ni);
        info_indices[i] = ni - &ctx->nodes_info[0];
        pipeline_size += ni->size_bytes;
    }

    char *node_it = um_alloc_persistent(ctx, pipeline_size);

    for (int i = 0; i < node_count; ++i) {
        umugu_node *n = (void *)node_it;
        ctx->pipeline.nodes[i] = n;
        n->info_idx = info_indices[i];
        n->prev_node = i - 1;
        umugu_node_dispatch(ctx, n, UMUGU_FN_INIT, UMUGU_FN_INIT_DEFAULTS);
        node_it += ctx->nodes_info[n->info_idx].size_bytes;
    }
    return UMUGU_SUCCESS;
}

#endif // __UMUGU_SERIALIZATION_H__
