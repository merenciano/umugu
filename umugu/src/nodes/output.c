#include "builtin_nodes.h"
#include "umugu.h"
#include "umutils.h"

#include <assert.h>

static inline int um__init(umugu_node *node) {
    node->out_pipe.samples = NULL;
    node->out_pipe.count = 0;
    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->out_pipe.samples) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    umugu_node *input = ctx->pipeline.nodes[node->in_pipe_node];
    if (!input->out_pipe.samples) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->out_pipe.samples);
    }

    umugu_generic_signal sigout = ctx->io.out_audio;
    node->out_pipe.samples = sigout.sample_data;
    /* TODO: SampleRate conversion if the output is different. */
    switch (sigout.type) {
    case UMUGU_TYPE_FLOAT: {
        float *out = sigout.sample_data;
        if (sigout.flags & UMUGU_SIGNAL_INTERLEAVED) {
            for (int i = 0; i < sigout.count; ++i) {
                for (int ch = 0; ch < sigout.channels; ++ch) {
                    *out++ = umu_signal_get_channel(input->out_pipe, ch)[i];
                }
            }
        } else {
            for (int ch = 0; ch < sigout.channels; ++ch) {
                for (int i = 0; i < sigout.count; ++i) {
                    *out++ = umu_signal_get_channel(input->out_pipe, ch)[i];
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
                    *out++ = umu_signal_get_channel(input->out_pipe, ch)[i] *
                             2147483648.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.channels; ++ch) {
                for (int i = 0; i < sigout.count; ++i) {
                    *out++ = umu_signal_get_channel(input->out_pipe, ch)[i] *
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
                    *out++ = umu_signal_get_channel(input->out_pipe, ch)[i] *
                             32768.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.channels; ++ch) {
                for (int i = 0; i < sigout.count; ++i) {
                    *out++ = umu_signal_get_channel(input->out_pipe, ch)[i] *
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
                        umu_signal_get_channel(input->out_pipe, ch)[i] * 128.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.channels; ++ch) {
                for (int i = 0; i < sigout.count; ++i) {
                    *out++ =
                        umu_signal_get_channel(input->out_pipe, ch)[i] * 128.0f;
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
                    *out++ = (umu_signal_get_channel(input->out_pipe, ch)[i] +
                              1.0f) *
                             128.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.channels; ++ch) {
                for (int i = 0; i < sigout.count; ++i) {
                    *out++ = (umu_signal_get_channel(input->out_pipe, ch)[i] +
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

    return UMUGU_SUCCESS;
}

umugu_node_fn um__output_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_PROCESS:
        return um__process;
    default:
        return NULL;
    }
}
