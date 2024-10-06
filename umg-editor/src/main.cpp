#include "umugu_editor.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <umugu/umugu.h>
#include <unistd.h>

static void um__assert(int cond)
{
    assert(cond);
}

int main()
{
    umugu_ctx* ctx = umugu_get_context();
    ctx->alloc = malloc;
    ctx->free = free;
    ctx->assert = um__assert;
    ctx->log = printf;
    umugu_editor::Init();
    while (!umugu_editor::PollEvents()) {
        umugu_editor::Render();
    }
    umugu_editor::Close();
    return 0;
}
