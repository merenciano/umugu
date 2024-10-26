#include "builtin_nodes.h"
#include "umugu.h"

#include <assert.h>
#include <portmidi.h>
#include <porttime.h>
#include <string.h>

#define UMUGU_MIDI_EVENT_CAPACITY 64

enum umugu_midi_message_ids {
    UMUGU_MIDI_NOTE_OFF = 0x80,
    UMUGU_MIDI_NOTE_ON = 0x90,
    UMUGU_MIDI_CONTROL = 0xB0,
    UMUGU_MIDI_PITCH_WHEEL = 0xE0,

    UMUGU_MIDI_PITCH_WHEEL_CENTER = 0x2000
};

static int um__pm_initialized = 0;

static inline int umugu_midi_event_status_channel(int msg) { return msg & 0xF; }
static inline int umugu_midi_event_status_status(int msg) { return msg & 0xF0; }
static inline int umugu_midi_event_byte0(int msg) { return msg & 0xFF; }
static inline int umugu_midi_event_byte1(int msg) { return (msg >> 8) & 0xFF; }
static inline int umugu_midi_event_byte2(int msg) { return (msg >> 16) & 0xFF; }
static inline int umugu_midi_event_byte3(int msg) { return (msg >> 24) & 0xFF; }

static inline int umugu_midi_process(umugu_ctrl *ctrl, int32_t msg) {
    int status = umugu_midi_event_status_status(msg);
    switch (status) {
    case UMUGU_MIDI_NOTE_OFF: {
        int note = umugu_midi_event_byte1(msg);
        ctrl->notes[note]--;
        return UMUGU_SUCCESS;
    }

    case UMUGU_MIDI_NOTE_ON: {
        int note = umugu_midi_event_byte1(msg);
        int vel = umugu_midi_event_byte2(msg);
        if (vel) {
            ctrl->notes[note]++;
        } else {
            ctrl->notes[note]--;
        }
        return UMUGU_SUCCESS;
    }

    case UMUGU_MIDI_CONTROL: {
        int ctrl_idx = umugu_midi_event_byte1(msg);
        int value = umugu_midi_event_byte2(msg);
        switch (ctrl_idx) {
        case 0x07: {
            ctrl->volume &= 0x7F;
            ctrl->volume |= (value << 7);
            break;
        }

        case 0x27: {
            ctrl->volume &= 0x3F80;
            ctrl->volume |= value;
            break;
        }

        case 0x30: {
            ctrl->ctrl[UMUGU_CTRL_GP1] &= 0x3F80;
            ctrl->ctrl[UMUGU_CTRL_GP1] |= value;
            break;
        }

        case 0x31: {
            ctrl->ctrl[UMUGU_CTRL_GP2] &= 0x3F80;
            ctrl->ctrl[UMUGU_CTRL_GP2] |= value;
            break;
        }

        case 0x32: {
            ctrl->ctrl[UMUGU_CTRL_GP3] &= 0x3F80;
            ctrl->ctrl[UMUGU_CTRL_GP3] |= value;
            break;
        }

        case 0x33: {
            ctrl->ctrl[UMUGU_CTRL_GP4] &= 0x3F80;
            ctrl->ctrl[UMUGU_CTRL_GP4] |= value;
            break;
        }

        case 0x42: {
            ctrl->flags &= ~UMUGU_CTRL_FLAG_SOSTENUTO;
            if (value >> 6) {
                ctrl->flags |= UMUGU_CTRL_FLAG_SOSTENUTO;
            }
            break;
        }

        case 0x43: {
            ctrl->flags &= ~UMUGU_CTRL_FLAG_SOFT_PEDAL;
            if (value >> 6) {
                ctrl->flags |= UMUGU_CTRL_FLAG_SOFT_PEDAL;
            }
            break;
        }

        case 0x46: {
            ctrl->sound[UMUGU_CTRL_SOUND_VARIATION] = value;
            break;
        }

        case 0x47: {
            ctrl->sound[UMUGU_CTRL_SOUND_TIMBRE] = value;
            break;
        }

        case 0x48: {
            ctrl->sound[UMUGU_CTRL_SOUND_RELEASE_TIME] = value;
            break;
        }

        case 0x49: {
            ctrl->sound[UMUGU_CTRL_SOUND_ATTACK_TIME] = value;
            break;
        }

        case 0x79: { /* Reset all controllers. */
            assert(value == 0 && "MIDI spec");
            memset(ctrl->ctrl, 0, sizeof(ctrl->ctrl));
            ctrl->flags = 0;
            break;
        }

        case 0x7B: { /* All notes off. */
            assert(value == 0 && "MIDI spec");
            memset(ctrl->notes, 0, sizeof(ctrl->notes));
            break;
        }
        }

        return UMUGU_SUCCESS;
    }

    case UMUGU_MIDI_PITCH_WHEEL: {
        int least = umugu_midi_event_byte1(msg);
        int most = umugu_midi_event_byte2(msg);
        int pitch = (least | (most << 7)) & 0x3FFF;
        ctrl->pitch = pitch - UMUGU_MIDI_PITCH_WHEEL_CENTER;
        return UMUGU_SUCCESS;
    }

    default:
        return UMUGU_NOOP;
    }
}

static inline int um__init(umugu_node *node) {
    umugu_ctx *ctx = umugu_get_context();
    um__ctrlmidi *self = (void *)node;
    node->pipe_out_type = UMUGU_PIPE_CONTROL;
    node->pipe_out_ready = 0;

    if (!um__pm_initialized) {
        Pm_Initialize();
        Pt_Start(1, NULL, NULL);
        um__pm_initialized = 1;
    }

    self->stream = NULL;
    if (*self->dev_name != 0) {
        for (int i = 0; i < Pm_CountDevices(); ++i) {
            const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
            if (info->input && !strcmp(self->dev_name, info->name)) {
                self->dev_idx = i;
            }
        }
    } else {
        self->dev_idx = Pm_GetDefaultInputDeviceID();
    }

    PmError err =
        Pm_OpenInput(&self->stream, self->dev_idx, NULL, 4, NULL, NULL);
    if (err) {
        ctx->io.log("PortMidi Error: %s\n", Pm_GetErrorText(err));
        Pt_Stop();
        self->stream = NULL;
        return UMUGU_ERR_MIDI;
    }

    ctx->io.log("MIDI: Input stream opened for device %s (%d)\n",
                self->dev_name, self->dev_idx);

    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->pipe_out_ready) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    um__ctrlmidi *self = (void *)node;
    if (!self->stream) {
        return UMUGU_NOOP;
    }

    PmEvent events[UMUGU_MIDI_EVENT_CAPACITY];
    int count = Pm_Read(self->stream, events, UMUGU_MIDI_EVENT_CAPACITY);

    if (count < 0) {
        ctx->io.log("PortMidi Error: %s\n", Pm_GetErrorText((PmError)count));
        return UMUGU_ERR_MIDI;
    }

    for (int i = 0; i < count; ++i) {
        umugu_midi_process(&ctx->pipeline.control, events[i].message);
    }

    node->pipe_out_ready = 1;

    return UMUGU_SUCCESS;
}

static inline int um__release(umugu_node *node) {
    um__ctrlmidi *self = (void *)node;

    Pm_Close(self->stream);
    self->stream = NULL;

    if (um__pm_initialized) {
        Pt_Stop();
        Pm_Terminate();
        um__pm_initialized = 0;
    }

    return UMUGU_SUCCESS;
}

umugu_node_fn um__ctrlmidi_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_PROCESS:
        return um__process;
    case UMUGU_FN_RELEASE:
        return um__release;
    default:
        return NULL;
    }
}
