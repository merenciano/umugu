
#include <umugu/umugu.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ARENA_SIZE (1024 * 1024)
static char buffer[ARENA_SIZE];

int main(void) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->io.log = printf;
    umugu_set_arena(buffer, ARENA_SIZE);

    umugu_generate_test_pipeline("../assets/pipelines/auto.bin");
    umugu_import_pipeline("../assets/pipelines/auto.bin");

    return 0;
}
