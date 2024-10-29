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

    umugu_pipeline_generate(NODES, sizeof(NODES) / sizeof(*NODES));
    umugu_audio_backend_play(60000);
    return 0;
}
