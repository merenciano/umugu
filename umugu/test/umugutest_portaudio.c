#include <umugu/umugu.h>

#define UMUGU_PORTAUDIO19_IMPL
#include <umugu/backends/umugu_portaudio19.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define C_HZ       (32 * 8)
#define C_SHARP_HZ (34 * 8)
#define D_HZ       (36 * 8)
#define D_SHARP_HZ (38 * 8)
#define E_HZ       (41 * 8)
#define F_HZ       (43 * 8)
#define F_SHARP_HZ (46 * 8)
#define G_HZ       (49 * 8)
#define G_SHARP_HZ (52 * 8)
#define A_HZ       (55 * 8)
#define A_SHARP_HZ (58 * 8)
#define B_HZ       (61 * 8)

#define NUM_KEYS 13

static int32_t g_values[NUM_KEYS] = {
    C_HZ,
    C_SHARP_HZ,
    D_HZ,
    D_SHARP_HZ,
    E_HZ,
    F_HZ,
    F_SHARP_HZ,
    G_HZ,
    G_SHARP_HZ,
    A_HZ,
    A_SHARP_HZ,
    B_HZ,
    C_HZ * 2      
};

int main(int argc, char **argv) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->alloc = malloc;
    ctx->free = free;
    ctx->io.log = printf;

    umugu_init();
    umugu_audio_backend_init();

    if (argc > 1) {
        umugu_load_pipeline_bin(argv[1]);
    } else {
        umugu_load_pipeline_bin("../assets/pipelines/synth.bin");
    }

    umugu_name mixname = {.str = "Mixer"};
    umugu_name oscname = {.str = "Oscilloscope"};
    ptrdiff_t base_offset = umugu_node_info_find(&mixname)->size_bytes;
    ptrdiff_t osc_sz = umugu_node_info_find(&oscname)->size_bytes;

    int32_t maps[NUM_KEYS] =
        {'a', 'w', 's', 'e', 'd', 'f', 't', 'g', 'y', 'h', 'u', 'j', 'k'};
    for (int i = 0; i < NUM_KEYS; ++i) {
        ctx->io.in.keys[i].mapped_key = maps[i];
        ctx->io.in.keys[i].node_pipeline_offset = base_offset + (osc_sz * i);
        ctx->io.in.keys[i].var_idx = 0;
        ctx->io.in.keys[i].value.i = 0;
    }
    ctx->io.in.keys[0].value.i = 440;

    umugu_audio_backend_start_stream();

    int key = 0;
    int prev_key = 0;
    while (key != '0') {
        key = getchar();
        for (int i = 0; i < NUM_KEYS; ++i) {
            if (ctx->io.in.keys[i].mapped_key == key) {
                ctx->io.in.keys[i].value.i = g_values[i];
                ctx->io.in.dirty_keys |= (1UL << i);
            }
        }

        for (int i = 0; i < NUM_KEYS; ++i) {
            if (ctx->io.in.keys[i].mapped_key == prev_key) {
                ctx->io.in.keys[i].value.i = 0;
                ctx->io.in.dirty_keys |= (1UL << i);
            }
        }

        prev_key = key;
    }

    umugu_audio_backend_stop_stream();
    umugu_close();

    return 0;
}
