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
static int um__pm_channel_mask = Pm_Channel(0);

static void um__pm_callback(PtTimestamp time, void *userdata) {
    (void)time;
    umugu_midi_poll((umugu_ctx*)userdata);
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
    Pm_SetChannelMask(um__pm_stream, um__pm_channel_mask);
    ctx->io.midi.input_handle = um__pm_stream;
}

int umugu_midi_poll(umugu_ctx *ctx) {
    umugu_midi *midi = &ctx->io.midi;
    if (!midi->input_enabled) {
        return;
    }

    int count;
    while ((count = Pm_Read(um__pm_stream, (PmEvent *)midi->events,
                            UMUGU_MIDI_INPUT_EVENTS_CAPACITY))) {
        if (count < 0) {
            ctx->io.log("PortMidi Error: %s\n", Pm_GetErrorText((PmError)count));
            break;
        }

        for (int i = 0; i < count; ++i) {
            int msg = midi->events[i].msg;
            int msg_status = Pm_MessageStatus(msg);
            int status = msg_status & (0xF << 4);
            int channel = msg_status & 0xF;
            int data1 = Pm_MessageData1(msg);
            int data2 = Pm_MessageData2(msg);
            switch (status) { 
                case UM__MIDI_NOTE_OFF: {
                    midi->channels[channel].notes[data1]--;
                    break;
                }
                case UM__MIDI_NOTE_ON: {
                    if (!data2) {
                        /* Note off */
                        midi->channels[channel].notes[data1]--;
                    } else {
                        midi->channels[channel].notes[data1]++;
                    }
                    break;
                }
                default: {
                    ctx->io.log("PortMidi event not implemented: %d\n", msg_status);
                }
            }
        }
    }
}

#endif /* UMUGU_PORTMIDI_IMPL */
