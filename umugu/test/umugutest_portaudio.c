#include <umugu/umugu.h>

#define UMUGU_PORTAUDIO19_IMPL
#include <umugu/backends/umugu_portaudio19.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
        umugu_load_pipeline_bin("../assets/pipelines/plugtest.bin");
    }

    umugu_audio_backend_start_stream();
    while (getchar()) {
    }
    umugu_audio_backend_stop_stream();
    umugu_close();

    return 0;
}
