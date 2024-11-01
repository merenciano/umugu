#include <umugu/umugu.h>
#define UMUGU_STDOUT_IMPL
#include <umugu/backends/umugu_stdout.h>

#include <stdio.h>
#include <unistd.h>

static const umugu_name NODES[] = {{.str = "WavFilePlayer"}, {.str = "Output"}};

int main(void) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->io.log = printf;
    char arena[999999];
    umugu_set_arena(arena, 999999);
    ctx->io.out_audio.channels = 2;
    ctx->io.out_audio.flags = UMUGU_SIGNAL_ENABLED | UMUGU_SIGNAL_INTERLEAVED;
    ctx->io.out_audio.rate = 48000;
    ctx->io.out_audio.type = UMUGU_TYPE_FLOAT;
    ctx->io.in_audio.flags = 0;

    umugu_pipeline_generate(NODES, sizeof(NODES) / sizeof(*NODES));
    umugu_audio_backend_play(60000);
    return 0;
}
