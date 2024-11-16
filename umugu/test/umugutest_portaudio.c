#include "../src/debugu.h"

#include <umugu/umugu.h>

#define UMUGU_PORTAUDIO19_IMPL
#include <umugu/backends/umugu_portaudio19.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ARENA_SIZE (1024 * 1024)
static char arena_buffer[ARENA_SIZE];

int
main(int argc, char **argv)
{
    umugu_set_arena(arena_buffer, ARENA_SIZE);
    umugu_ctx *ctx = umugu_get_context();
    ctx->io.log = printf;

    umugu_config *conf = &ctx->config;
    conf->flags |= UMUGU_CONFIG_SCAN_MIDI_AUTO;
    conf->flags = UMUGU_CONFIG_DEBUG;
    conf->sample_rate = 48000;
    // strcpy(conf->default_midi_device, "Minilab3 Minilab3 MIDI");
    strcpy(conf->default_midi_device, "hw:Minilab3");
    strcpy(conf->default_audio_file, "../assets/audio/undertale.sf2");

    umugu_generic_signal *sig = &ctx->io.out_audio;
    sig->channels = 2;
    sig->flags = UMUGU_SIGNAL_ENABLED | UMUGU_SIGNAL_INTERLEAVED;
    sig->rate = conf->sample_rate;
    sig->type = UMUGU_TYPE_FLOAT;

    umugu_init();

    if (argc == 2) {
        umugu_import_pipeline(argv[1]);
    } else {
        const umugu_name nodes[] = {{.str = "SoundFont"}, {.str = "Output"}};
        umugu_pipeline_generate(&nodes[0], sizeof(nodes) / sizeof(*nodes));
    }

    umugu_audio_backend_init();
    umugu_audio_backend_start_stream();

    ctx->io.log(
        "Blocking main thread (audio is being processed in another thread via "
        "callbacks)\nPress any key and 'Enter' for closing...\n");
    getchar();

    umugu_audio_backend_stop_stream();

    umugu_mem_arena_print();
    umugu_in_signal_print();
    umugu_pipeline_print();
    umugu_out_signal_print();

    return 0;
}
