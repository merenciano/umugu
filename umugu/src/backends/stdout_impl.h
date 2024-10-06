#ifndef __STDOUT_IMPL_H__
#define __STDOUT_IMPL_H__

#include "../umugu_internal.h"
#include "umugu/umugu.h"

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

void um__write_wav_header(size_t wav_data_size) {
    wav_header hdr = {
        .riff = "RIFF",
        .bytes_remaining = wav_data_size + 36,
        .wave = "WAVE",
        .fmt = "fmt ",
        .num16 = 16,
        .sample_fmt = UMUGU_SAMPLE_TYPE == UMUGU_TYPE_FLOAT ? 3 : 1,
        .channels = UMUGU_CHANNELS,
        .sample_rate = UMUGU_SAMPLE_RATE,
        .bytes_per_sec =
            UMUGU_SAMPLE_RATE * UMUGU_CHANNELS * sizeof(umugu_sample),
        .bytes_per_frame = UMUGU_CHANNELS * sizeof(umugu_sample),
        .bits_per_sample = sizeof(umugu_sample) * 8, // 8: byte -> bits
        .data = "data",
        .data_size = wav_data_size};

    /* 44 is the specified size of the .wav header */
    umugu_get_context()->assert(sizeof(hdr) == 44);
    fwrite(&hdr, sizeof(hdr), 1, stdout);
}

int um__backend_start_stream(void) {
    enum { SAMPLE_COUNT = UMUGU_DEFAULT_SAMPLE_CAPACITY };
    umugu_frame frames[SAMPLE_COUNT];
    umugu_ctx *ctx = umugu_get_context();
    umugu_pipeline *graph = &ctx->pipeline;
    ctx->audio_output.output = (umugu_signal){.frames = frames,
                                              .count = SAMPLE_COUNT,
                                              .rate = UMUGU_SAMPLE_RATE,
                                              .type = UMUGU_SAMPLE_TYPE,
                                              .channels = UMUGU_CHANNELS,
                                              .capacity = SAMPLE_COUNT};

    size_t count = SAMPLE_COUNT * SAMPLE_COUNT;
    um__write_wav_header(count);
    while (count -= 1024) {
        umugu_node *it = graph->root;
        umugu_node_call(UMUGU_FN_GETSIGNAL, &it, &(ctx->audio_output.output));

        fwrite(frames, SAMPLE_COUNT * sizeof(umugu_frame), 1, stdout);
    }
    return UMUGU_SUCCESS;
}

int um__backend_stop_stream(void) { return UMUGU_SUCCESS; }

int um__backend_init(void) { return UMUGU_SUCCESS; }

int um__backend_close(void) { return UMUGU_SUCCESS; }

#endif // __STDOUT_IMPL_H__
