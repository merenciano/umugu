
#include <umugu/umugu.h>

#define UMUGU_PORTMIDI_IMPL
#include <umugu/backends/umugu_portmidi.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->alloc = malloc;
    ctx->free = free;
    ctx->io.log = printf;

    umugu_init();

    ctx->io.midi.input_device_name = "Minilab3 Minilab3 MIDI";
    umugu_midi_init();
    umugu_midi_start_stream();

    int err;
    while ((err = umugu_midi_poll())) {
        if (err == UMUGU_ERR_MIDI) {
            printf("Midi error, aborting...\n");
            break;
        }
    }

    umugu_midi_stop_stream();
    umugu_midi_close();
    umugu_close();

    return 0;
}
