#ifndef __UMUGU_STDOUT_H__
#define __UMUGU_STDOUT_H__

#endif /* __UMUGU_STDOUT_H__ */

#define UMUGU_STDOUT_IMPL
#ifdef UMUGU_STDOUT_IMPL

#include "umugu/umugu.h"
#include "umugu/umugu_internal.h"

#include <stdio.h>

int
umugu_audio_backend_play(umugu_ctx *ctx, int milliseconds)
{
    enum { SAMPLE_COUNT = 2048 };
    char samples[SAMPLE_COUNT * 4];

    if (!ctx->io.out_audio.samples.channel_count) {
        return UMUGU_NOOP;
    }

    ctx->io.backend_name = "stdout_pipe_wav";

    int frames_left = (int)(ctx->io.out_audio.sample_rate / 1000) * milliseconds;
    UMUGU_ASSERT(frames_left >= 0);
    char buffer[44];
    um_signal_wav_header(&ctx->io.out_audio, frames_left, buffer);
    fwrite(buffer, 44, 1, stdout);

    while (frames_left) {
        int frame_count = frames_left > SAMPLE_COUNT ? SAMPLE_COUNT : frames_left;
        frames_left -= frame_count;
        ctx->io.out_audio.samples.samples = (void *)samples;
        ctx->io.out_audio.samples.frame_count = frame_count;

        umugu_process(ctx, frame_count);
        fwrite(
            samples,
            frame_count * um_type_sizeof(ctx->io.out_audio.format) *
                ctx->io.out_audio.samples.channel_count,
            1, stdout);
    }

    return UMUGU_SUCCESS;
}

#endif /* UMUGU_STDOUT_IMPL */
