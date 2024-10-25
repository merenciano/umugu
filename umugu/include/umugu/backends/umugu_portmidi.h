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
#include <string.h>

static PmStream *um__pm_in_stream = NULL;
static PmDeviceID um__pm_in_dev = pmNoDevice;

static void um__pm_callback(PtTimestamp time, void *userdata) {
    (void)time;
    (void)userdata;
    umugu_midi_poll();
}

int umugu_midi_init(void) {
    umugu_ctx *ctx = umugu_get_context();

    Pm_Initialize();

    ctx->io.log("MIDI: Device list:\n");
    for (int i = 0; i < Pm_CountDevices(); ++i) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        ctx->io.log("\t%d = %s [%s] %s.\n", i,
                    info->input    ? "IN "
                    : info->output ? "OUT"
                                   : "   ",
                    info->interf, info->name);
        if (info->input && ctx->io.midi.input_device_name) {
            if (!strcmp(ctx->io.midi.input_device_name, info->name)) {
                um__pm_in_dev = i;
            }
        }
    }
    ctx->io.log("\n");

    return UMUGU_SUCCESS;
}

int umugu_midi_start_stream(void) {
    umugu_ctx *ctx = umugu_get_context();
    if (um__pm_in_stream) {
        if (ctx->io.midi.input_handle != um__pm_in_stream) {
            ctx->io.log("PortMidi Error: Midi input stream already opened but "
                        "not the one in umugu context.\n");
            return UMUGU_ERR_MIDI;
        }
        ctx->io.log("PortMidi: Ignored start midi input stream: the stream is "
                    "already opened.\n");
        return UMUGU_NOOP;
    }

    if (um__pm_in_dev == pmNoDevice) {
        um__pm_in_dev = Pm_GetDefaultInputDeviceID();
    }

    Pt_Start(1, um__pm_callback, ctx);
    PmError err =
        Pm_OpenInput(&um__pm_in_stream, um__pm_in_dev, NULL, 4, NULL, NULL);
    if (err) {
        ctx->io.log("PortMidi Error: %s\n", Pm_GetErrorText(err));
        Pt_Stop();
        ctx->io.midi.input_handle = NULL;
        return UMUGU_ERR_MIDI;
    }

    ctx->io.log("MIDI: Input stream opened for device %s (%d)\n",
                ctx->io.midi.input_device_name, um__pm_in_dev);
    ctx->io.midi.input_handle = um__pm_in_stream;

    return UMUGU_SUCCESS;
}

int umugu_midi_stop_stream(void) {
    umugu_ctx *ctx = umugu_get_context();
    Pm_Close(um__pm_in_stream);
    Pt_Stop();
    ctx->io.midi.input_handle = NULL;
    return UMUGU_SUCCESS;
}

int umugu_midi_close(void) {
    Pm_Terminate();
    return UMUGU_SUCCESS;
}

int umugu_midi_poll(void) {
    umugu_ctx *ctx = umugu_get_context();
    if (!ctx->io.midi.input_handle) {
        return UMUGU_NOOP;
    }

    umugu_midi_event events[UMUGU_MIDI_INPUT_EVENTS_CAPACITY];
    int count = Pm_Read(um__pm_in_stream, (PmEvent *)events,
                        UMUGU_MIDI_INPUT_EVENTS_CAPACITY);

    if (count < 0) {
        ctx->io.log("PortMidi Error: %s\n", Pm_GetErrorText((PmError)count));
        return UMUGU_ERR_MIDI;
    }

    for (int i = 0; i < count; ++i) {
        umugu_midi_process(events[i]);
    }

    return UMUGU_SUCCESS;
}

#endif /* UMUGU_PORTMIDI_IMPL */
