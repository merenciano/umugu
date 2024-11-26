#ifndef __UMUGU_STDOUT_H__
#define __UMUGU_STDOUT_H__

#endif /* __UMUGU_STDOUT_H__ */

#define UMUGU_STDOUT_IMPL
#ifdef UMUGU_STDOUT_IMPL

#include "../src/umugu_internal.h"
#include "umugu/umugu.h"

#include <stdio.h>

/* Generic interface. */
int umugu_audio_backend_play(umugu_ctx *ctx, int milliseconds);

typedef struct {
    char riff[4]; /* "RIFF" */
    /* Total wave data size + header (44) - current index (8) */
    int32_t bytes_remaining;
    char wave[4];       /* "WAVE" */
    char fmt[4];        /* "fmt " mind the space (0x20 char) */
    int32_t num16;      /* just assign the number 16 */
    int16_t sample_fmt; /* 1: Int, 3: Float */
    int16_t channels;
    int32_t sample_rate;
    int32_t bytes_per_sec;   /* sample_rate * sample_size * number_channels */
    int16_t bytes_per_frame; /* sample_size * number_channels */
    int16_t bits_per_sample; /* i.e. sample_size * 8 */
    char data[4];            /* "data" */
    int32_t data_size;
} wav_header;

static inline int
um__type_size(int type)
{
    switch (type) {
    case UMUGU_TYPE_UINT8:
        return 1;
    case UMUGU_TYPE_INT16:
        return 2;
    case UMUGU_TYPE_INT32:
        return 4;
    case UMUGU_TYPE_FLOAT:
        return 4;
    default:
        UMUGU_ASSERT(0);
        return 0;
    }
}

void
um__write_wav_header(umugu_ctx *ctx, size_t wav_data_size)
{
    umugu_signal *sig = &ctx->pipeline.sig;
    wav_header hdr = {
        .riff = "RIFF",
        .bytes_remaining = wav_data_size + 36,
        .wave = "WAVE",
        .fmt = "fmt ",
        .num16 = 16,
        .sample_fmt = sig->format == UMUGU_TYPE_FLOAT ? 3 : 1,
        .channels = sig->samples.channel_count,
        .sample_rate = sig->sample_rate,
        .bytes_per_sec = sig->sample_rate * sig->samples.channel_count * um__type_size(sig->format),
        .bytes_per_frame = sig->samples.channel_count * um__type_size(sig->format),
        .bits_per_sample = um__type_size(sig->format) * 8, // 8: byte -> bits
        .data = "data",
        .data_size = wav_data_size};

    /* 44 is the specified size of the .wav header */
    UMUGU_ASSERT(sizeof(hdr) == 44);
    fwrite(&hdr, sizeof(hdr), 1, stdout);
}

int
umugu_audio_backend_play(umugu_ctx *ctx, int milliseconds)
{
    enum { SAMPLE_COUNT = 2048 };
    char samples[SAMPLE_COUNT * 4];

    if (!ctx->io.backend.audio_output) {
        return UMUGU_NOOP;
    }
    umugu_signal *sig = &ctx->pipeline.sig;
    sig->samples.channel_count = ctx->io.backend.channel_count;
    sig->sample_rate = ctx->io.backend.sample_rate;
    sig->format = ctx->io.backend.format;
    sig->interleaved_channels = ctx->io.backend.interleaved_channels;

    ctx->io.backend.backend_name = "stdout_pipe_wav";
    ctx->io.backend.backend_running = true;

    int frames_left = (int)(sig->sample_rate / 1000) * milliseconds;
    UMUGU_ASSERT(frames_left >= 0);
    um__write_wav_header(ctx, frames_left);

    while (frames_left) {
        int frame_count = frames_left > SAMPLE_COUNT ? SAMPLE_COUNT : frames_left;
        frames_left -= frame_count;
        sig->samples.samples = (void *)samples;
        sig->samples.frame_count = frame_count;

        ctx->io.backend.audio_callback(ctx, frame_count, NULL, sig);
        fwrite(samples, frame_count * um__type_size(sig->format) * sig->samples.channel_count, 1,
               stdout);
    }

    ctx->io.backend.backend_running = false;

    return UMUGU_SUCCESS;
}

#endif /* UMUGU_STDOUT_IMPL */
