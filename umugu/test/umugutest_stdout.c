#include <umugu/umugu.h>
#define UMUGU_STDOUT_IMPL
#include <umugu/backends/umugu_stdout.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    umugu_ctx *ctx = umugu_get_context();
    ctx->alloc = malloc;
    ctx->free = free;
    ctx->log = printf;

	umugu_init();
    umugu_load_pipeline_bin("../assets/pipelines/plugtest.bin");

    umugu_audio_backend_play(60000);

    umugu_close();
    return 0;
}