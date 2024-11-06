#include "umutils.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

umu_nanosec umu_time_now(void) {
    struct timespec time;
    clock_gettime(2, &time);
    return time.tv_nsec + time.tv_sec * 1000000000;
}

void umu_gen_waveform_file(const char *filename) {
    FILE *f = fopen(filename, "w");

    fprintf(
        f,
        "#include \"umugu.h\"\n\nconst float "
        "umugu_osciloscope_lut[UMUGU_WAVEFORM_COUNT][UMUGU_SAMPLE_RATE] = {\n");
    for (int i = 0; i < UMUGU_WAVEFORM_COUNT; ++i) {
        fprintf(f, "{");
        for (int j = 0; j < UMUGU_SAMPLE_RATE; ++j) {
            fprintf(f, "%ff, ", umugu_waveform_lut[i][j]);
        }
        fprintf(f, "},\n");
    }
    fprintf(f, "};\n");
    fclose(f);
}

void umu_fill_waveform_lut(void) {
    const float linear_step = (1.0f / (UMUGU_SAMPLE_RATE / 4.0f));

    // Triangle wave
    float lerp = 0.0f;
    for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i) {
        float cos_value = cosf((i / (float)UMUGU_SAMPLE_RATE) * M_PI * 2.0f);
        *(float *)&umugu_waveform_lut[UMUGU_WAVEFORM_TRIANGLE][i] = lerp;
        lerp += (cos_value > 0.0f ? linear_step : -linear_step);
    }

    // Sine and square waves
    for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i) {
        float sine_value = sinf((i / (float)UMUGU_SAMPLE_RATE) * M_PI * 2.0f);
        *(float *)&umugu_waveform_lut[UMUGU_WAVEFORM_SINE][i] = sine_value;
        *(float *)&umugu_waveform_lut[UMUGU_WAVEFORM_SQUARE][i] =
            sine_value > 0.0f ? 1.0f : -1.0f;
    }

    // Saw wave
    for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i) {
        *(float *)&umugu_waveform_lut[UMUGU_WAVEFORM_SAW][i] =
            (float)i / (UMUGU_SAMPLE_RATE / 2.0f);
        if (i > (UMUGU_SAMPLE_RATE / 2.0f)) {
            *(float *)&umugu_waveform_lut[UMUGU_WAVEFORM_SAW][i] -= 2.0f;
        }
    }

    // White noise
    int g_x1 = 0x67452301;
    int g_x2 = 0xefcdab89;
    for (int i = 0; i < UMUGU_SAMPLE_RATE; ++i) {
        g_x1 ^= g_x2;
        *(float *)&umugu_waveform_lut[UMUGU_WAVEFORM_WHITE_NOISE][i] =
            g_x2 * (2.0f / (float)0xffffffff);
        g_x2 += g_x1;
    }
}
