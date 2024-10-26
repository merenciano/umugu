#include <umugu/umugu.h>

#define UMUGU_PORTAUDIO19_IMPL
#include <umugu/backends/umugu_portaudio19.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ARENA_SIZE (1024 * 1024 * 1024)
static char arena_buffer[ARENA_SIZE];

int main(int argc, char **argv) {
    umugu_set_arena(arena_buffer, ARENA_SIZE);
    umugu_get_context()->io.log = printf;
    umugu_init();

    if (argc == 2) {
        umugu_import_pipeline(argv[1]);
    } else {
        umugu_generate_test_pipeline("../assets/pipeines/default.bin");
        umugu_import_pipeline("../assets/pipelines/default.bin");
    }

    umugu_audio_backend_init();
    umugu_audio_backend_start_stream();

    getchar();

    umugu_audio_backend_stop_stream();
    umugu_close();

    return 0;
}
