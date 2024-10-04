#include <umugu/umugu.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

static void dummy_assert(int cond)
{
    (void)cond;
}

static void dummy_print(int lvl, const char *fmt, ...)
{
    (void)lvl; (void)fmt;
}

int main(int argc, char **argv)
{
    umugu_ctx *ctx = umugu_get_context();
    ctx->alloc = malloc;
    ctx->free = free;
    ctx->abort = abort;
    ctx->assert = dummy_assert;
    ctx->log = dummy_print;

	umugu_init();
    if (argc > 1) {
        umugu_load_pipeline_bin(argv[1]);
    } else {
        umugu_load_pipeline_bin("../assets/pipelines/littlewing.bin");
    }

	umugu_start_stream();

    sleep(60);

	return 0;
}
