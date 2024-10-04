#include "umugu.h"
#include "umugu_internal.h"

/* Needed for waveform LUT generation. */
#include <stdio.h>
#include <math.h>
#include <string.h>

static inline void um__init(umugu_node **node, umugu_signal *_)
{
    um__unused(_);
    umugu_ctx *ctx = umugu_get_context();
    um__oscope *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__oscope));
    self->sample_capacity = ctx->audio_src_sample_capacity;
    self->audio_source = ctx->alloc(self->sample_capacity * sizeof(umugu_sample));
}

static inline void um__getsignal(umugu_node **node, umugu_signal *out)
{
    umugu_ctx *ctx = umugu_get_context();
    um__oscope *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__oscope));
    const int count = out->count;
    if (count > self->sample_capacity) {
        self->sample_capacity *= 2;
        if (self->sample_capacity < 256) {
            self->sample_capacity = 256;
        }

        ctx->free(self->audio_source);
        self->audio_source = ctx->alloc(self->sample_capacity * sizeof(umugu_sample));
        if (!self->audio_source) {
            ctx->err = UMUGU_ERR_MEM;
            strncpy(ctx->err_msg, "Error: BadAlloc. Ignoring Wav player this frame.", UMUGU_ERR_MSG_LEN);
            self->sample_capacity = 0;
            self->audio_source = NULL; 
            return;
        }
    }

    for (int i = 0; i < count; i++) {
        float sample = um__waveform_lut[self->waveform][self->phase];
        self->audio_source[i].left = sample;
        self->audio_source[i].right = sample;
        self->phase += self->freq;
        if (self->phase >= ctx->sample_rate) {
            self->phase -= ctx->sample_rate;
        }
    }
    out->samples = self->audio_source;
}

umugu_node_fn um__oscope_getfn(umugu_code fn)
{
    switch (fn) {
        case UMUGU_FN_INIT: return um__init;
        case UMUGU_FN_GETSIGNAL: return um__getsignal;
        default: return NULL;
    }
}

void um__gen_wave_file(const char *filename)
{
    FILE* f = fopen(filename, "w");

    fprintf(f, "#include \"umugu.h\"\n\nconst float umugu_osciloscope_lut[UM__WAVEFORM_COUNT][UMUGU_SAMPLE_RATE] = {\n");
    for (int i = 0; i < UM__WAVEFORM_COUNT; ++i) {
        fprintf(f, "{");
        for (int j = 0; j < UMUGU_SAMPLE_RATE; ++j) {
            fprintf(f, "%ff, ", um__waveform_lut[i][j]);
        }
        fprintf(f, "},\n");
    }
    fprintf(f, "};\n");
    fclose(f);
}

void um__fill_wave_lut(void)
{
    const float linear_step = (1.0f / (UMUGU_SAMPLE_RATE / 4.0f));

    // Triangle wave
    float lerp = 0.0f;
    for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i) {
        float cos_value = cosf((i / (float)UMUGU_SAMPLE_RATE) * M_PI * 2.0f);
        *(float*)&um__waveform_lut[UM__WAVEFORM_TRIANGLE][i] = lerp;
        lerp += (cos_value > 0.0f ? linear_step : -linear_step);
    }

    // Sine and square waves
    for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i) {
        float sine_value = sinf((i / (float)UMUGU_SAMPLE_RATE) * M_PI * 2.0f);
        *(float*)&um__waveform_lut[UM__WAVEFORM_SINE][i] = sine_value;
        *(float*)&um__waveform_lut[UM__WAVEFORM_SQUARE][i] = sine_value > 0.0f ? 1.0f : -1.0f;
    }

    // Saw wave
    for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i) {
        *(float*)&um__waveform_lut[UM__WAVEFORM_SAW][i] = (float)i / (UMUGU_SAMPLE_RATE / 2.0f);
        if (i > (UMUGU_SAMPLE_RATE / 2.0f)) {
            *(float*)&um__waveform_lut[UM__WAVEFORM_SAW][i] -= 2.0f;
        }
    }

    // White noise
    int g_x1 = 0x67452301;
    int g_x2 = 0xefcdab89;
    for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i) {
        g_x1 ^= g_x2;
        *(float*)&um__waveform_lut[UM__WAVEFORM_WHITE_NOISE][i] = g_x2 * (2.0f / (float)0xffffffff); 
        g_x2 += g_x1;
    }
}
