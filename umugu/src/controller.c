#include "controller.h"

#include "umugu/umugu.h"

#include <assert.h>
#include <string.h>

int umugu_ctrl_init(void);
int umugu_ctrl_update(void);
int umugu_ctrl_close(void);

int umugu_ctrl_midi_init(void);
int umugu_ctrl_midi_update(void);
int umugu_ctrl_midi_close(void);

int umugu_ctrl_init(void) {
    umugu_ctx *ctx = umugu_get_context();
    if (ctx->config.flags & UMUGU_CONFIG_SCAN_MIDI_AUTO) {
        strncpy(ctx->io.controller.device_name, ctx->config.default_midi_device,
                UMUGU_PATH_LEN);
        umugu_ctrl_midi_init();
    }

    return UMUGU_SUCCESS;
}

int umugu_ctrl_update(void) {
    umugu_ctx *ctx = umugu_get_context();
    umugu_ctrl *ctrl = &ctx->io.controller;
    if (ctrl->data_stream && ctrl->device_idx >= 0) {
        int err = umugu_ctrl_midi_update();
        if (err < 0) {
            ctx->io.log("[Error] umugu_ctrl_midi_update returned %d.\n", err);
            return err;
        }
    }

    return UMUGU_SUCCESS;
}

int umugu_ctrl_close(void) {
    int ret = UMUGU_SUCCESS;
    umugu_ctx *ctx = umugu_get_context();
    umugu_ctrl *ctrl = &ctx->io.controller;
    if (ctrl->data_stream && ctrl->device_idx >= 0) {
        int err = umugu_ctrl_midi_close();
        if (err < 0) {
            ctx->io.log("[Error] umugu_ctrl_midi_update returned %d.\n", err);
            /* Not returning here since I don't care about handling closing
             * errors. */
            ret = err;
        }
    }

    return ret;
}

#ifndef UMUGU_DISABLE_MIDI

#include <portmidi.h>
#include <porttime.h>

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
// static inline int umugu_midi_event_byte0(int msg) { return msg & 0xFF; }
static inline int umugu_midi_event_byte1(int msg) { return (msg >> 8) & 0xFF; }
static inline int umugu_midi_event_byte2(int msg) { return (msg >> 16) & 0xFF; }
// static inline int umugu_midi_event_byte3(int msg) { return (msg >> 24) &
// 0xFF; }

static inline int umugu_ctrl_midi_decode(int32_t msg) {
    umugu_ctx *ctx = umugu_get_context();
    umugu_ctrl *ctrl = &ctx->io.controller;
    int status = umugu_midi_event_status_status(msg);
    switch (status) {
    case UMUGU_MIDI_NOTE_OFF: {
        int note = umugu_midi_event_byte1(msg);
        if (umugu_midi_event_status_channel(msg) == 0x9) {
            ctrl->drums[note - 35] = 0;
        } else {
            ctrl->notes[note] = 0;
        }

        return UMUGU_SUCCESS;
    }

    case UMUGU_MIDI_NOTE_ON: {
        int note = umugu_midi_event_byte1(msg);
        int vel = umugu_midi_event_byte2(msg);
        if (umugu_midi_event_status_channel(msg) == 0x9) {
            ctrl->drums[note - 35] = vel;
        } else {
            ctrl->notes[note] = vel;
        }
        return UMUGU_SUCCESS;
    }

    case UMUGU_MIDI_CONTROL: {
        int ctrl_idx = umugu_midi_event_byte1(msg);
        int value = umugu_midi_event_byte2(msg);
        switch (ctrl_idx) {
        case 0x07: {
            ctrl->values[UMUGU_CTRL_VOLUME] = value;
            break;
        }

        case 0x0C: {
            ctrl->values[UMUGU_CTRL_FX1] = value;
            break;
        }

        case 0x0D: {
            ctrl->values[UMUGU_CTRL_FX2] = value;
            break;
        }

        case 0x0E: {
            ctrl->values[UMUGU_CTRL_5] = value;
            break;
        }

        case 0x0F: {
            ctrl->values[UMUGU_CTRL_6] = value;
            break;
        }

        case 0x10: {
            ctrl->values[UMUGU_CTRL_1] = value;
            break;
        }

        case 0x11: {
            ctrl->values[UMUGU_CTRL_2] = value;
            break;
        }

        case 0x12: {
            ctrl->values[UMUGU_CTRL_3] = value;
            break;
        }

        case 0x13: {
            ctrl->values[UMUGU_CTRL_4] = value;
            break;
        }

        case 0x46: {
            ctrl->values[UMUGU_CTRL_VARIATION] = value;
            break;
        }

        case 0x47: {
            ctrl->values[UMUGU_CTRL_TIMBRE] = value;
            break;
        }

        case 0x48: {
            ctrl->values[UMUGU_CTRL_RELEASE_TIME] = value;
            break;
        }

        case 0x49: {
            ctrl->values[UMUGU_CTRL_ATTACK_TIME] = value;
            break;
        }

        case 0x4A: {
            ctrl->values[UMUGU_CTRL_BRIGHTNESS] = value;
            break;
        }

        case 0x4B: {
            ctrl->values[UMUGU_CTRL_SOUND_6] = value;
            break;
        }

        case 0x4C: {
            ctrl->values[UMUGU_CTRL_SOUND_7] = value;
            break;
        }

        case 0x4D: {
            ctrl->values[UMUGU_CTRL_SOUND_8] = value;
            break;
        }

        case 0x5D: {
            ctrl->values[UMUGU_CTRL_CHORUS_DEPTH] = value;
            break;
        }
        }

        return UMUGU_SUCCESS;
    }

    case UMUGU_MIDI_PITCH_WHEEL: {
        int least = umugu_midi_event_byte1(msg);
        int most = umugu_midi_event_byte2(msg);
        int pitch = (least | (most << 7)) & 0x3FFF;
        ctrl->values[UMUGU_CTRL_PITCHWHEEL] =
            pitch - UMUGU_MIDI_PITCH_WHEEL_CENTER;
        return UMUGU_SUCCESS;
    }

    default:
        return UMUGU_NOOP;
    }
}

int umugu_ctrl_midi_init(void) {
    umugu_ctx *ctx = umugu_get_context();
    umugu_ctrl *ctrl = &ctx->io.controller;
    if (!um__pm_initialized) {
        Pm_Initialize();
        Pt_Start(1, NULL, NULL);
        um__pm_initialized = 1;
    }

    ctrl->data_stream = NULL;
    if (*ctrl->device_name != 0) {
        for (int i = 0; i < Pm_CountDevices(); ++i) {
            const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
            ctx->io.log("Midi device: %s %s.\n", info->name,
                        info->input ? "Input" : "Output");
            if (info->input && !strcmp(ctrl->device_name, info->name)) {
                ctrl->device_idx = i;
            }
        }
    } else {
        ctrl->device_idx = Pm_GetDefaultInputDeviceID();
    }

    PmError err =
        Pm_OpenInput(&ctrl->data_stream, ctrl->device_idx, NULL, 4, NULL, NULL);
    if (err) {
        ctx->io.log("PortMidi Error: %s\n", Pm_GetErrorText(err));
        Pt_Stop();
        ctrl->data_stream = NULL;
        return UMUGU_ERR_MIDI;
    }

    ctx->io.log("MIDI: Input stream opened for device %s (%d)\n",
                ctrl->device_name, ctrl->device_idx);

    return UMUGU_SUCCESS;
}

int umugu_ctrl_midi_update(void) {
    umugu_ctx *ctx = umugu_get_context();
    umugu_ctrl *ctrl = &ctx->io.controller;

    if (!ctrl->data_stream) {
        return UMUGU_NOOP;
    }

    PmEvent events[UMUGU_MIDI_EVENT_CAPACITY];
    int count = Pm_Read(ctrl->data_stream, events, UMUGU_MIDI_EVENT_CAPACITY);

    if (count < 0) {
        ctx->io.log("PortMidi Error: %s\n", Pm_GetErrorText((PmError)count));
        return UMUGU_ERR_MIDI;
    }

    for (int i = 0; i < count; ++i) {
        umugu_ctrl_midi_decode(events[i].message);
    }

    return UMUGU_SUCCESS;
}

int umugu_ctrl_midi_close(void) {
    umugu_ctx *ctx = umugu_get_context();
    umugu_ctrl *ctrl = &ctx->io.controller;
    Pm_Close(ctrl->data_stream);
    ctrl->data_stream = NULL;

    if (um__pm_initialized) {
        Pt_Stop();
        Pm_Terminate();
        um__pm_initialized = 0;
    }

    return UMUGU_SUCCESS;
}

#elif defined(UMUGU_DISABLE_MIDI) /* UMUGU_DISABLE_MIDI */

int umugu_ctrl_midi_init(void) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->io.log("Trying to initialize the MIDI interface with "
                "UMUGU_DISABLE_MIDI defined. Ignoring call...\n");
    return UMUGU_NOOP;
}

int umugu_ctrl_midi_update(void) { return UMUGU_NOOP; }
int umugu_ctrl_midi_close(void) { return UMUGU_NOOP; }

#endif /* UMUGU_DISABLE_MIDI */
