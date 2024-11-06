#include "../src/debugu.h"

#include <umugu/umugu.h>

#define UMUGU_PORTAUDIO19_IMPL
#include <umugu/backends/umugu_portaudio19.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ARENA_SIZE (1024 * 1024)
static char arena_buffer[ARENA_SIZE];

int main(int argc, char **argv) {
    umugu_set_arena(arena_buffer, ARENA_SIZE);
    umugu_ctx *ctx = umugu_get_context();
    ctx->io.log = printf;

    umugu_generic_signal *sig = &ctx->io.out_audio;
    sig->channels = 2;
    sig->flags = UMUGU_SIGNAL_ENABLED | UMUGU_SIGNAL_INTERLEAVED;
    sig->rate = UMUGU_SAMPLE_RATE;
    sig->type = UMUGU_TYPE_FLOAT;

    umugu_init();

    if (argc == 2) {
        umugu_import_pipeline(argv[1]);
    } else {
        const umugu_name nodes[] = {{.str = "WavFilePlayer"},
                                    {.str = "Amplitude"},
                                    {.str = "Limiter"},
                                    {.str = "Output"}};
        umugu_pipeline_generate(&nodes[0], sizeof(nodes) / sizeof(*nodes));
    }

    int offset_filename =
        ctx->nodes_info[ctx->pipeline.nodes[0]->info_idx].vars[3].offset_bytes;
    char *filename = offset_filename + (char *)ctx->pipeline.nodes[0];
    strncpy(filename, "../assets/audio/littlewingmono.wav", UMUGU_PATH_LEN);
    umugu_node_dispatch(ctx->pipeline.nodes[0], UMUGU_FN_INIT);

    umugu_audio_backend_init();
    umugu_audio_backend_start_stream();

    getchar();

    umugu_audio_backend_stop_stream();

    umugu_mem_arena_print();
    umugu_in_signal_print();
    umugu_pipeline_print();
    umugu_out_signal_print();

    return 0;
}
