#ifndef __UMUGU_PORTMIDI_H__
#define __UMUGU_PORTMIDI_H__

#ifdef __cplusplus
extern "C" {
#endif
int umugu_midi_init(void);
int umugu_midi_close(void);
int umugu_midi_start_stream(void);
int umugu_midi_stop_stream(void);
int umugu_midi_poll(void);
#ifdef __cplusplus
}
#endif

#endif /* __UMUGU_PORTMIDI_H__ */

#define UMUGU_PORTMIDI_IMPL
#ifdef UMUGU_PORTMIDI_IMPL

#include <umugu/umugu.h>

#include <portmidi.h>
#include <porttime.h>

#define UM__MIDI_NOTE_OFF 0x80
#define UM__MIDI_NOTE_ON 0x90

static PmStream *um__pm_stream = NULL;
static PmDeviceID um__pm_dev = -1;
static int um__pm_filter_input = PM_FILT_NOTE;

static void um__pm_callback(PtTimestamp time, void *userdata) {
    (void)time;
    umugu_ctx *ctx = userdata;
    umugu_midi *midi = &ctx->io.midi;
    if (!midi->input_enabled) {
        return;
    }

    int count;
    while ((count = Pm_Read(um__pm_stream, (PmEvent *)midi->events,
                            UMUGU_MIDI_INPUT_EVENTS_CAPACITY))) {
        if (count < 0) {
            ctx->io.log("PortMidi Error: %s\n", Pm_GetErrorText(count));
            break;
        }

        for (int i = 0; i < count; ++i) {
            int event = Pm_MessageStatus(midi->events[i]);
            switch (event) { case UM__MIDI_NOTE_OFF: }
            asdasdd
        }
    }
}

int umugu_midi_init(void) {
    umugu_ctx *ctx = umugu_get_context();
    Pt_Start(1, um__pm_callback, ctx);
    Pm_Initialize();

    for (int i = 0; i < Pm_CountDevices(); ++i) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (info->input && !info->opened) {
            ctx->io.log("MIDI: Selected input device: %s.\n", info->name);
            um__pm_dev = i;
            break;
        }
    }
}

int umugu_midi_start_stream(void) {
    umugu_ctx *ctx = umugu_get_context();
    if (ctx->io.midi.input_handle == um__pm_stream) {
        ctx->io.log("PortMidi: Ignored start midi input stream: the stream is "
                    "already opened.\n");
        return UMUGU_NOOP;
    }

    PmError err = Pm_OpenInput(&um__pm_stream, um__pm_dev, NULL,
                               UMUGU_MIDI_INPUT_EVENTS_CAPACITY, NULL, NULL);
    if (err) {
        ctx->io.log("PortMidi Error: %s\n", Pm_GetErrorText(err));
        Pt_Stop();
        ctx->io.midi.input_handle = NULL;
        return UMUGU_ERR_MIDI;
    }
    Pm_SetFilter(um__pm_stream, um__pm_filter_input);
    ctx->io.midi.input_handle = um__pm_stream;
}

#endif /* UMUGU_PORTMIDI_IMPL */
