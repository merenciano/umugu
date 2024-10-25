#ifndef __UMUGU_MIDI_H__
#define __UMUGU_MIDI_H__

#include <stdint.h>

#define UMUGU_MIDI_CHANNEL_COUNT 16
#define UMUGU_MIDI_CONTROL_COUNT 128
#define UMUGU_MIDI_SYSEX_BYTES 32

#ifndef UMUGU_MIDI_INPUT_EVENTS_CAPACITY
#define UMUGU_MIDI_INPUT_EVENTS_CAPACITY 127
#endif

enum umugu_midi_control_ids {
    UMUGU_MIDI_CTRL_BANK_SELECT = 0,
    UMUGU_MIDI_CTRL_MOD_WHEEL,
    UMUGU_MIDI_CTRL_BREATH,
    UMUGU_MIDI_CTRL_FOOT,
    UMUGU_MIDI_CTRL_PORTAMENTO_TIME,
    UMUGU_MIDI_CTRL_DATA,
    UMUGU_MIDI_CTRL_BALANCE,
    UMUGU_MIDI_CTRL_PAN,
    UMUGU_MIDI_CTRL_EXPRESSION,
    UMUGU_MIDI_CTRL_FX1,
    UMUGU_MIDI_CTRL_FX2,
    UMUGU_MIDI_CTRL_GENERIC1,
    UMUGU_MIDI_CTRL_GENERIC2,
    UMUGU_MIDI_CTRL_GENERIC3,
    UMUGU_MIDI_CTRL_GENERIC4,
    UMUGU_MIDI_CTRL_GENERIC5,
    UMUGU_MIDI_CTRL_GENERIC6,
    UMUGU_MIDI_CTRL_GENERIC7,
    UMUGU_MIDI_CTRL_GENERIC8,
    UMUGU_MIDI_CTRL_SOUND1, /* Variation. */
    UMUGU_MIDI_CTRL_SOUND2, /* Timbre. */
    UMUGU_MIDI_CTRL_SOUND3, /* Release time. */
    UMUGU_MIDI_CTRL_SOUND4, /* Attack time. */
    UMUGU_MIDI_CTRL_SOUND5, /* Brightness. */
    UMUGU_MIDI_CTRL_SOUND6,
    UMUGU_MIDI_CTRL_SOUND7,
    UMUGU_MIDI_CTRL_SOUND8,
    UMUGU_MIDI_CTRL_SOUND9,
    UMUGU_MIDI_CTRL_SOUND10,
    UMUGU_MIDI_CTRL_PORTAMENTO_NOTE,
    UMUGU_MIDI_CTRL_DEPTH_FX1,
    UMUGU_MIDI_CTRL_DEPTH_FX2,
    UMUGU_MIDI_CTRL_DEPTH_FX3,
    UMUGU_MIDI_CTRL_DEPTH_FX4,
    UMUGU_MIDI_CTRL_DEPTH_FX5,
    UMUGU_MIDI_CTRL_UNREG_PARAM_NUM,
    UMUGU_MIDI_CTRL_REG_PARAM_NUM,

    UMUGU_MIDI_CTRL_COUNT
};

enum umugu_midi_ctrl_flags {
    UMUGU_MIDI_CTRL_FLAG_DAMPER_PEDAL = 1,
    UMUGU_MIDI_CTRL_FLAG_PORTAMENTO = 1 << 1,
    UMUGU_MIDI_CTRL_FLAG_SOSTENUTO = 1 << 2,
    UMUGU_MIDI_CTRL_FLAG_SOFT_PEDAL = 1 << 3,
    UMUGU_MIDI_CTRL_FLAG_LEGATO_FOOTSWITCH = 1 << 4,
    UMUGU_MIDI_CTRL_FLAG_HOLD2 = 1 << 5,
};

enum umugu_midi_note_definitions {
    UMUGU_NOTE_CMINUS1 = 0,
    UMUGU_NOTE_CSHARPMINUS1,
    UMUGU_NOTE_DMINUS1,
    UMUGU_NOTE_DSHARPMINUS1,
    UMUGU_NOTE_EMINUS1,
    UMUGU_NOTE_FMINUS1,
    UMUGU_NOTE_FSHARPMINUS1,
    UMUGU_NOTE_GMINUS1,
    UMUGU_NOTE_GSHARPMINUS1,
    UMUGU_NOTE_AMINUS1,
    UMUGU_NOTE_ASHARPMINUS1,
    UMUGU_NOTE_BMINUS1,

    UMUGU_NOTE_C0,
    UMUGU_NOTE_CSHARP0,
    UMUGU_NOTE_D0,
    UMUGU_NOTE_DSHARP0,
    UMUGU_NOTE_E0,
    UMUGU_NOTE_F0,
    UMUGU_NOTE_FSHARP0,
    UMUGU_NOTE_G0,
    UMUGU_NOTE_GSHARP0,
    UMUGU_NOTE_A0,
    UMUGU_NOTE_ASHARP0,
    UMUGU_NOTE_B0,

    UMUGU_NOTE_C1,
    UMUGU_NOTE_CSHARP1,
    UMUGU_NOTE_D1,
    UMUGU_NOTE_DSHARP1,
    UMUGU_NOTE_E1,
    UMUGU_NOTE_F1,
    UMUGU_NOTE_FSHARP1,
    UMUGU_NOTE_G1,
    UMUGU_NOTE_GSHARP1,
    UMUGU_NOTE_A1,
    UMUGU_NOTE_ASHARP1,
    UMUGU_NOTE_B1,

    UMUGU_NOTE_C2,
    UMUGU_NOTE_CSHARP2,
    UMUGU_NOTE_D2,
    UMUGU_NOTE_DSHARP2,
    UMUGU_NOTE_E2,
    UMUGU_NOTE_F2,
    UMUGU_NOTE_FSHARP2,
    UMUGU_NOTE_G2,
    UMUGU_NOTE_GSHARP2,
    UMUGU_NOTE_A2,
    UMUGU_NOTE_ASHARP2,
    UMUGU_NOTE_B2,

    UMUGU_NOTE_C3,
    UMUGU_NOTE_CSHARP3,
    UMUGU_NOTE_D3,
    UMUGU_NOTE_DSHARP3,
    UMUGU_NOTE_E3,
    UMUGU_NOTE_F3,
    UMUGU_NOTE_FSHARP3,
    UMUGU_NOTE_G3,
    UMUGU_NOTE_GSHARP3,
    UMUGU_NOTE_A3,
    UMUGU_NOTE_ASHARP3,
    UMUGU_NOTE_B3,

    UMUGU_NOTE_C4,
    UMUGU_NOTE_CSHARP4,
    UMUGU_NOTE_D4,
    UMUGU_NOTE_DSHARP4,
    UMUGU_NOTE_E4,
    UMUGU_NOTE_F4,
    UMUGU_NOTE_FSHARP4,
    UMUGU_NOTE_G4,
    UMUGU_NOTE_GSHARP4,
    UMUGU_NOTE_A4,
    UMUGU_NOTE_ASHARP4,
    UMUGU_NOTE_B4,

    UMUGU_NOTE_C5,
    UMUGU_NOTE_CSHARP5,
    UMUGU_NOTE_D5,
    UMUGU_NOTE_DSHARP5,
    UMUGU_NOTE_E5,
    UMUGU_NOTE_F5,
    UMUGU_NOTE_FSHARP5,
    UMUGU_NOTE_G5,
    UMUGU_NOTE_GSHARP5,
    UMUGU_NOTE_A5,
    UMUGU_NOTE_ASHARP5,
    UMUGU_NOTE_B5,

    UMUGU_NOTE_C6,
    UMUGU_NOTE_CSHARP6,
    UMUGU_NOTE_D6,
    UMUGU_NOTE_DSHARP6,
    UMUGU_NOTE_E6,
    UMUGU_NOTE_F6,
    UMUGU_NOTE_FSHARP6,
    UMUGU_NOTE_G6,
    UMUGU_NOTE_GSHARP6,
    UMUGU_NOTE_A6,
    UMUGU_NOTE_ASHARP6,
    UMUGU_NOTE_B6,

    UMUGU_NOTE_C7,
    UMUGU_NOTE_CSHARP7,
    UMUGU_NOTE_D7,
    UMUGU_NOTE_DSHARP7,
    UMUGU_NOTE_E7,
    UMUGU_NOTE_F7,
    UMUGU_NOTE_FSHARP7,
    UMUGU_NOTE_G7,
    UMUGU_NOTE_GSHARP7,
    UMUGU_NOTE_A7,
    UMUGU_NOTE_ASHARP7,
    UMUGU_NOTE_B7,

    UMUGU_NOTE_C8,
    UMUGU_NOTE_CSHARP8,
    UMUGU_NOTE_D8,
    UMUGU_NOTE_DSHARP8,
    UMUGU_NOTE_E8,
    UMUGU_NOTE_F8,
    UMUGU_NOTE_FSHARP8,
    UMUGU_NOTE_G8,
    UMUGU_NOTE_GSHARP8,
    UMUGU_NOTE_A8,
    UMUGU_NOTE_ASHARP8,
    UMUGU_NOTE_B8,

    UMUGU_NOTE_C9,
    UMUGU_NOTE_CSHARP9,
    UMUGU_NOTE_D9,
    UMUGU_NOTE_DSHARP9,
    UMUGU_NOTE_E9,
    UMUGU_NOTE_F9,
    UMUGU_NOTE_FSHARP9,
    UMUGU_NOTE_G9,

    UMUGU_NOTE_COUNT // This has to be 128
};

enum umugu_midi_mode_ids {
    UMUGU_MIDI_MODE_DEFAULT,
    UMUGU_MIDI_MODE_OMNI,
    UMUGU_MIDI_MODE_POLY,
};

typedef struct {
    int16_t ctrl[UMUGU_MIDI_CTRL_COUNT];
    uint32_t ctrl_flags;
    int8_t notes[UMUGU_NOTE_COUNT];
    int16_t pitch;
    int16_t volume;
    int16_t pan;
} umugu_midi_channel;

typedef struct {
    int32_t msg;
    int32_t time; // milliseconds
} umugu_midi_event;

typedef struct {
    /* Midi abstraction layer */
    umugu_midi_channel channels[UMUGU_MIDI_CHANNEL_COUNT];
    uint8_t sysex[UMUGU_MIDI_SYSEX_BYTES];
    int16_t sysex_next;
    int16_t song_curr_beat; /* Song position pointer. 1 beat = 6 MIDI clocks. */
    int8_t song;
    int8_t is_song_playing;
    int8_t is_tune_requested;
    int8_t mode; /* 0: Default, 1: Omni, 2: Poly. */

    /* Implementation data */
    void *input_handle;
    const char *input_device_name;
} umugu_midi;

/* MIDI message parsing helpers. */
static inline int umugu_midi_event_status_channel(int msg) { return msg & 0xF; }
static inline int umugu_midi_event_status_status(int msg) { return msg & 0xF0; }
static inline int umugu_midi_event_byte0(int msg) { return msg & 0xFF; }
static inline int umugu_midi_event_byte1(int msg) { return (msg >> 8) & 0xFF; }
static inline int umugu_midi_event_byte2(int msg) { return (msg >> 16) & 0xFF; }
static inline int umugu_midi_event_byte3(int msg) { return (msg >> 24) & 0xFF; }

int umugu_midi_process(umugu_midi_event event);

#endif /* __UMUGU_MIDI_H__ */
