/* Rename to resample. */

#include "umugu.h"
#include "umugu_internal.h"

#include <assert.h>

static inline int
um_output_init(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(ctx), UM_UNUSED(flags);
    node->out_pipe.samples = NULL;
    node->out_pipe.frame_count = 0;
    return UMUGU_SUCCESS;
}

static inline int
um_output_process(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(flags);
    const umugu_node *input = um_node_get_input(ctx, node);

    umugu_signal sigout = ctx->io.out_audio;
    node->out_pipe.samples = sigout.samples.samples;
    /* TODO: SampleRate conversion if the output is different. */
    switch (sigout.format) {
    case UMUGU_TYPE_FLOAT: {
        float *out = sigout.samples.samples;
        if (sigout.interleaved_channels) {
            for (int i = 0; i < sigout.samples.frame_count; ++i) {
                for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i];
                }
            }
        } else {
            for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                for (int i = 0; i < sigout.samples.frame_count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i];
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_INT32: {
        int32_t *out = (void *)sigout.samples.samples;
        if (sigout.interleaved_channels) {
            for (int i = 0; i < sigout.samples.frame_count; ++i) {
                for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 2147483648.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                for (int i = 0; i < sigout.samples.frame_count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 2147483648.0f;
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_INT16: {
        int16_t *out = (void *)sigout.samples.samples;
        if (sigout.interleaved_channels) {
            for (int i = 0; i < sigout.samples.frame_count; ++i) {
                for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 32768.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                for (int i = 0; i < sigout.samples.frame_count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 32768.0f;
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_INT8: {
        int8_t *out = (void *)sigout.samples.samples;
        if (sigout.interleaved_channels) {
            for (int i = 0; i < sigout.samples.frame_count; ++i) {
                for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 128.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                for (int i = 0; i < sigout.samples.frame_count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 128.0f;
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_UINT8: {
        uint8_t *out = (void *)sigout.samples.samples;
        if (sigout.interleaved_channels) {
            for (int i = 0; i < sigout.samples.frame_count; ++i) {
                for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                    *out++ = (um_signal_get_channel(&input->out_pipe, ch)[i] + 1.0f) * 128.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                for (int i = 0; i < sigout.samples.frame_count; ++i) {
                    *out++ = (um_signal_get_channel(&input->out_pipe, ch)[i] + 1.0f) * 128.0f;
                }
            }
        }
        break;
    }
    default:
        ctx->io.log("[ERR] Output: invalid sample data type.");
        break;
    }

    return UMUGU_SUCCESS;
}

umugu_node_func
um_output_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_output_init;
    case UMUGU_FN_PROCESS:
        return um_output_process;
    default:
        return NULL;
    }
}
