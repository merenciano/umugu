#include <umugu/umugu.h>

#define UMUGU_PORTAUDIO19_IMPL
#include <umugu/backends/umugu_portaudio19.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ARENA_SIZE (1024 * 1024 * 1024)

int main(int argc, char **argv) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->arena.buffer = malloc(ARENA_SIZE);
    ctx->arena.capacity = ARENA_SIZE;
    ctx->arena.pers_next = ctx->arena.buffer;
    ctx->arena.temp_next = ctx->arena.buffer;
    ctx->alloc = malloc;
    ctx->free = free;
    ctx->io.log = printf;

    umugu_init();
    umugu_audio_backend_init();

    if (argc > 1) {
        umugu_import_pipeline(argv[1]);
    } else {
        umugu_import_pipeline("../assets/pipelines/auto.bin");
    }

    umugu_audio_backend_start_stream();

    getchar();

    umugu_audio_backend_stop_stream();
    umugu_close();

    return 0;
}
