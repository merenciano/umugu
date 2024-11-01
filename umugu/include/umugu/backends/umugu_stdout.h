#ifndef __UMUGU_STDOUT_H__
#define __UMUGU_STDOUT_H__

/* Generic interface. */
int umugu_audio_backend_play(int milliseconds);

#endif /* __UMUGU_STDOUT_H__ */

#define UMUGU_STDOUT_IMPL
#ifdef UMUGU_STDOUT_IMPL

#include "umugu/umugu.h"

#include <assert.h>
#include <stdio.h>

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

static inline int um__type_size(int type) {
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
        assert(0);
        return 0;
    }
}

void um__write_wav_header(size_t wav_data_size) {
    umugu_ctx *ctx = umugu_get_context();
    umugu_generic_signal *sig = &ctx->io.out_audio;
    wav_header hdr = {
        .riff = "RIFF",
        .bytes_remaining = wav_data_size + 36,
        .wave = "WAVE",
        .fmt = "fmt ",
        .num16 = 16,
        .sample_fmt = sig->type == UMUGU_TYPE_FLOAT ? 3 : 1,
        .channels = sig->channels,
        .sample_rate = sig->rate,
        .bytes_per_sec = sig->rate * sig->channels * um__type_size(sig->type),
        .bytes_per_frame = sig->channels * um__type_size(sig->type),
        .bits_per_sample = um__type_size(sig->type) * 8, // 8: byte -> bits
        .data = "data",
        .data_size = wav_data_size};

    /* 44 is the specified size of the .wav header */
    assert(sizeof(hdr) == 44);
    fwrite(&hdr, sizeof(hdr), 1, stdout);
}

int umugu_audio_backend_play(int milliseconds) {
    enum { SAMPLE_COUNT = 2048 };
    char samples[SAMPLE_COUNT * 4];

    umugu_ctx *ctx = umugu_get_context();
    umugu_generic_signal *sig = &ctx->io.out_audio;

    int frames_left = (int)(sig->rate / 1000) * milliseconds;
    assert(frames_left >= 0);
    um__write_wav_header(frames_left);

    while (frames_left) {
        int frame_count =
            frames_left > SAMPLE_COUNT ? SAMPLE_COUNT : frames_left;
        frames_left -= frame_count;
        sig->sample_data = (void *)samples;
        sig->count = frame_count;

        umugu_newframe();
        for (int i = 0; i < ctx->pipeline.node_count; ++i) {
            umugu_node_dispatch(ctx->pipeline.nodes[0], UMUGU_FN_PROCESS);
        }

        fwrite(samples, frame_count * um__type_size(sig->type) * sig->channels,
               1, stdout);
    }

    return UMUGU_SUCCESS;
}

#endif /* UMUGU_STDOUT_IMPL */
