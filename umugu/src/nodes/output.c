#include "umugu.h"
#include "umugu_internal.h"

#include <assert.h>

static inline int
um__init(umugu_node *node)
{
    node->out_pipe.samples = NULL;
    node->out_pipe.count = 0;
    return UMUGU_SUCCESS;
}

static inline int
um__defaults(umugu_node *node)
{
    (void)node;
    return UMUGU_SUCCESS;
}

static inline int
um__process(umugu_node *node)
{
    umugu_ctx *ctx = umugu_get_context();
    if (node->iteration >= ctx->pipeline_iteration) {
        /* Like the assert below, the pipeline should guarantee
           this can't happen. */
        UMUGU_TRAP();
        return UMUGU_NOOP;
    }

    umugu_node *input = ctx->pipeline.nodes[node->prev_node];
    assert(input->iteration > node->iteration);
#if 0 /* The pipeline should guarantee the correct node processing order. */
    if (input->iteration <= node->iteration) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
    }
#endif

    umugu_generic_signal sigout = ctx->io.out_audio;
    node->out_pipe.samples = sigout.sample_data;
    /* TODO: SampleRate conversion if the output is different. */
    switch (sigout.type) {
    case UMUGU_TYPE_FLOAT: {
        float *out = sigout.sample_data;
        if (sigout.flags & UMUGU_SIGNAL_INTERLEAVED) {
            for (int i = 0; i < sigout.count; ++i) {
                for (int ch = 0; ch < sigout.channels; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i];
                }
            }
        } else {
            for (int ch = 0; ch < sigout.channels; ++ch) {
                for (int i = 0; i < sigout.count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i];
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_INT32: {
        int32_t *out = sigout.sample_data;
        if (sigout.flags & UMUGU_SIGNAL_INTERLEAVED) {
            for (int i = 0; i < sigout.count; ++i) {
                for (int ch = 0; ch < sigout.channels; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] *
                             2147483648.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.channels; ++ch) {
                for (int i = 0; i < sigout.count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] *
                             2147483648.0f;
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_INT16: {
        int16_t *out = sigout.sample_data;
        if (sigout.flags & UMUGU_SIGNAL_INTERLEAVED) {
            for (int i = 0; i < sigout.count; ++i) {
                for (int ch = 0; ch < sigout.channels; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] *
                             32768.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.channels; ++ch) {
                for (int i = 0; i < sigout.count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] *
                             32768.0f;
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_INT8: {
        int8_t *out = sigout.sample_data;
        if (sigout.flags & UMUGU_SIGNAL_INTERLEAVED) {
            for (int i = 0; i < sigout.count; ++i) {
                for (int ch = 0; ch < sigout.channels; ++ch) {
                    *out++ =
                        um_signal_get_channel(&input->out_pipe, ch)[i] * 128.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.channels; ++ch) {
                for (int i = 0; i < sigout.count; ++i) {
                    *out++ =
                        um_signal_get_channel(&input->out_pipe, ch)[i] * 128.0f;
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_UINT8: {
        uint8_t *out = sigout.sample_data;
        if (sigout.flags & UMUGU_SIGNAL_INTERLEAVED) {
            for (int i = 0; i < sigout.count; ++i) {
                for (int ch = 0; ch < sigout.channels; ++ch) {
                    *out++ = (um_signal_get_channel(&input->out_pipe, ch)[i] +
                              1.0f) *
                             128.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.channels; ++ch) {
                for (int i = 0; i < sigout.count; ++i) {
                    *out++ = (um_signal_get_channel(&input->out_pipe, ch)[i] +
                              1.0f) *
                             128.0f;
                }
            }
        }
        break;
    }
    default:
        umugu_get_context()->io.log("[ERR] Output: invalid sample data type.");
        break;
    }

    node->iteration = ctx->pipeline_iteration;
    return UMUGU_SUCCESS;
}

umugu_node_fn
um_output_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_DEFAULTS:
        return um__defaults;
    case UMUGU_FN_PROCESS:
        return um__process;
    default:
        return NULL;
    }
}
