#include <umugu/umugu.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

static void um__assert(int cond)
{
    assert(cond);
}

int main(int argc, char **argv)
{
    umugu_ctx *ctx = umugu_get_context();
    ctx->alloc = malloc;
    ctx->free = free;
    ctx->assert = um__assert;
    ctx->log = printf;

	umugu_init();
    if (argc > 1) {
        umugu_load_pipeline_bin(argv[1]);
    } else {
        umugu_load_pipeline_bin("../assets/pipelines/plugtest.bin");
    }

	umugu_start_stream();
    sleep(60);
    umugu_stop_stream();
    umugu_close();

	return 0;
}
