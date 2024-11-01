#ifndef __UMUGU_SERIALIZATION_H__
#define __UMUGU_SERIALIZATION_H__

#include "builtin_nodes.h"
#include "umugu/umugu.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define UMUGU_HEAD_CODE 0x00454549
#define UMUGU_FOOT_CODE 0x00555541

#define UMUGU_GET_VERSION                                                      \
    ((UMUGU_VERSION_MAJOR << 20) | (UMUGU_VERSION_MINOR << 10) |               \
     UMUGU_VERSION_PATCH)

typedef struct {
    int32_t header;
    int32_t version;
    int32_t node_count;
} umugu_binary_header;

int umugu_export_pipeline(const char *filename, umugu_ctx *context) {
    umugu_ctx *ctx = context ? context : umugu_get_context();
    FILE *f = fopen(filename, "wb");
    if (!f) {
        ctx->io.log("Error: fopen('wb') failed with filename %s\n", filename);
        return UMUGU_ERR_FILE;
    }

    umugu_binary_header h = {.header = UMUGU_HEAD_CODE,
                             .version = UMUGU_GET_VERSION,
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

int umugu_import_pipeline(const char *filename) {
    umugu_ctx *ctx = umugu_get_context();
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

    if (h.version != UMUGU_GET_VERSION) {
        ctx->io.log(
            "Imported file's version mismatch. The format may have changed.\n"
            "File: %d.%d.%d, Current: %d.%d.%d.\n",
            h.version >> 20, (h.version >> 10) & 0x3FF, h.version & 0x3FF,
            UMUGU_VERSION_MAJOR, UMUGU_VERSION_MINOR, UMUGU_VERSION_PATCH);
    }

    umugu_name names[h.node_count];
    fread(&names, sizeof(umugu_name), h.node_count, f);
    ctx->pipeline.node_count = h.node_count;

    size_t node_buffer_bytes = 0;

    for (int i = 0; i < h.node_count; ++i) {
        const umugu_node_info *ni = umugu_node_info_load(&names[i]);
        /* Storing the offsets because I don't have the node_buffer yet. */
        ctx->pipeline.nodes[i] = (void *)node_buffer_bytes;
        node_buffer_bytes += ni->size_bytes;
    }

    char *node_buffer = umugu_alloc_pers(node_buffer_bytes);

    for (int i = 0; i < h.node_count; ++i) {
        /* Adding the buffer address to the previously cached offsets. */
        ctx->pipeline.nodes[i] =
            (void *)((size_t)ctx->pipeline.nodes[i] + node_buffer);
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
        umugu_node_dispatch(ctx->pipeline.nodes[i], UMUGU_FN_INIT);
    }
    return UMUGU_SUCCESS;
}

int umugu_pipeline_generate(const umugu_name *node_names, int node_count) {
    umugu_ctx *ctx = umugu_get_context();
    assert(ctx->pipeline.node_count == 0);
    ctx->pipeline.node_count = node_count;

    int info_indices[64];
    size_t pipeline_size = 0;

    for (int i = 0; i < node_count; ++i) {
        const umugu_node_info *ni = umugu_node_info_load(&node_names[i]);
        if (!ni) {
            ctx->io.log("Could not load node %s\n", node_names[i].str);
            return UMUGU_ERR;
        }
        info_indices[i] = ni - &ctx->nodes_info[0];
        pipeline_size += ni->size_bytes;
    }

    char *node_it = umugu_alloc_pers(pipeline_size);

    for (int i = 0; i < node_count; ++i) {
        umugu_node *n = (void *)node_it;
        ctx->pipeline.nodes[i] = n;
        n->info_idx = info_indices[i];
        n->in_pipe_node = i - 1;
        umugu_node_dispatch(n, UMUGU_FN_DEFAULTS);
        umugu_node_dispatch(n, UMUGU_FN_INIT);
        node_it += ctx->nodes_info[n->info_idx].size_bytes;
    }
    return UMUGU_SUCCESS;
}

#endif // __UMUGU_SERIALIZATION_H__
