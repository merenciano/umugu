
#include <umugu/umugu.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->alloc = malloc;
    ctx->free = free;
    ctx->io.log = printf;

    umugu_init();
    umugu_generate_test_pipeline("../assets/pipelines/auto.bin");
    umugu_import_pipeline("../assets/pipelines/auto.bin");

    umugu_close();

    return 0;
}
