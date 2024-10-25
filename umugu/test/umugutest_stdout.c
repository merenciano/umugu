#include <umugu/umugu.h>
#define UMUGU_STDOUT_IMPL
#include <umugu/backends/umugu_stdout.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->alloc = malloc;
    ctx->free = free;
    ctx->io.log = printf;

    umugu_init();
    umugu_import_pipeline("../assets/pipelines/plugtest.bin");

    umugu_audio_backend_play(60000);

    umugu_close();
    return 0;
}
