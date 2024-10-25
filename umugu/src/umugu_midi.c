#include "umugu/umugu.h"
#include "umugu/umugu_midi.h"

#include <assert.h>
#include <string.h>

enum umugu_midi_message_ids {
    UMUGU_MIDI_NOTE_OFF = 0x80,
    UMUGU_MIDI_NOTE_ON = 0x90,
    UMUGU_MIDI_CONTROL = 0xB0,
    UMUGU_MIDI_PITCH_WHEEL = 0xE0,
    UMUGU_MIDI_SYS = 0xF0,
    UMUGU_MIDI_SYSEX = 0xF0,
    UMUGU_MIDI_SYS_SONGPOS = 0xF2,
    UMUGU_MIDI_SYS_SONGSEL = 0xF3,
    UMUGU_MIDI_SYS_TUNE = 0xF6,
    UMUGU_MIDI_SYSEX_END = 0xF7,
    UMUGU_MIDI_SYSRT_CLOCK = 0xF8,
    UMUGU_MIDI_SYSRT_START = 0xFA,
    UMUGU_MIDI_SYSRT_CONTINUE = 0xFB,
    UMUGU_MIDI_SYSRT_STOP = 0xFC,
};

enum umugu_midi_manufacturer_ids {
    UMUGU_MIDI_MANUF_SEQUENTIAL_CIRCUITS = 0x1,
    UMUGU_MIDI_MANUF_MOOG = 0x04,
    UMUGU_MIDI_MANUF_FENDER = 0x08,
    UMUGU_MIDI_MANUF_STEINBERG = 0x3A,
    UMUGU_MIDI_MANUF_ROLAND = 0x41,
    UMUGU_MIDI_MANUF_KORG = 0x42,
    UMUGU_MIDI_MANUF_YAMAHA = 0x43,
    UMUGU_MIDI_MANUF_CASIO = 0x44,
    UMUGU_MIDI_MANUF_AKAI = 0x45,
    UMUGU_MIDI_MANUF_SONY = 0x4C,

    UMUGU_MIDI_MANUF_TEST = 0x7D,
    UMUGU_MIDI_MANUF_UNIV_NON_REALTIME = 0x7E,
    UMUGU_MIDI_MANUF_UNIV_REALTIME = 0x7F,

    UMUGU_MIDI_MANUF_BEHRINGER = 0x3220,
    UMUGU_MIDI_MANUF_MICROSOFT = 0x4100,
    UMUGU_MIDI_MANUF_ARTURIA = 0x6B20,
};

enum {
    UMUGU_MIDI_PITCH_WHEEL_CENTER = 0x2000
};

static inline int umugu_midi_process_sysex(uint8_t *sysex, int bytes) {
    if (sysex[0] != UMUGU_MIDI_SYSEX || sysex[bytes] != UMUGU_MIDI_SYSEX_END) {
        umugu_get_context()->io.log("MIDI ERR: Trying to parse an invalid System Exclusive message.\n");
        return UMUGU_ERR_MIDI_INPUT_SYSEX;
    }

    int16_t manuf = 0;
    if (sysex[1]) {
        /* Single byte manufacturer ID. */
        manuf = sysex[1];
        sysex = &sysex[2];
        bytes -= 2;
    } else {
        /* 3 byte manufacturer ID. */
        manuf = *(int16_t*)&sysex[2];
        sysex = &sysex[4];
        bytes -= 4;
    }

    switch (manuf) {
        case UMUGU_MIDI_MANUF_ARTURIA: {
            umugu_get_context()->io.log("DeviceID: %x, ProductID: %x\nMessage: ", sysex[4], sysex[5]);
            for (int i = 6; i < bytes; ++i) {
                umugu_get_context()->io.log("%x ", sysex[i]);
            }
            umugu_get_context()->io.log("\n");
            break;
        }

        default: {
            umugu_get_context()->io.log("MIDI: Manufacturer %x sysex not implemented.\n", manuf);
            break;
        }
    }

    return UMUGU_SUCCESS;
}

static inline int umugu_midi_process_byte(int byte) {
    umugu_midi *midi = &umugu_get_context()->io.midi;
    if (!byte && !midi->sysex_next) {
        return UMUGU_NOOP;
    }

    if (byte < 0x80) {
        /* SysEx data. */
        if (!midi->sysex_next) {
            /* ERROR.. non-zero byte out of sysex parsing. */
            assert(0);
            return UMUGU_ERR_MIDI;
        }

        midi->sysex[midi->sysex_next++] = byte;
        return UMUGU_SUCCESS;
    }

    switch(byte) {
        case UMUGU_MIDI_SYSEX: {
            midi->sysex_next = 0;
            midi->sysex[midi->sysex_next++] = byte;
            return UMUGU_SUCCESS;
        }

        case UMUGU_MIDI_SYSEX_END: {
            assert(midi->sysex_next && "SysEx end code needs an unfinished sysex message");

            midi->sysex[midi->sysex_next] = byte;
            umugu_midi_process_sysex(midi->sysex, midi->sysex_next);
            midi->sysex_next = 0;

            return UMUGU_SUCCESS;
        }

        case UMUGU_MIDI_SYSRT_CLOCK: {
            umugu_get_context()->io.log("MIDI: Msg clock.\n");
            return UMUGU_SUCCESS;
        }

        case UMUGU_MIDI_SYSRT_START: {
            midi->song_curr_beat = 0;
            midi->is_song_playing = 1;
            return UMUGU_SUCCESS;
        }

        case UMUGU_MIDI_SYSRT_CONTINUE: {
            midi->is_song_playing = 1;
            return UMUGU_SUCCESS;
        }

        case UMUGU_MIDI_SYSRT_STOP: {
            midi->is_song_playing = 0;
            return UMUGU_SUCCESS;
        }

        default: {
            umugu_get_context()->io.log("MIDI: Unimplemented message status %X.\n", byte);
            return UMUGU_NOOP;
        }
    }
}

int umugu_midi_process(umugu_midi_event event) {
    umugu_midi *midi = &umugu_get_context()->io.midi;
    int status = umugu_midi_event_status_status(event.msg);
    switch (status) {
        case UMUGU_MIDI_NOTE_OFF: {
            int ch = umugu_midi_event_status_channel(event.msg);
            int note = umugu_midi_event_byte1(event.msg);
            midi->channels[ch].notes[note]--;
            return UMUGU_SUCCESS;
        }

        case UMUGU_MIDI_NOTE_ON: {
            int ch = umugu_midi_event_status_channel(event.msg);
            int note = umugu_midi_event_byte1(event.msg);
            int vel = umugu_midi_event_byte2(event.msg);
            if (vel) {
                midi->channels[ch].notes[note]++;
            } else {
                midi->channels[ch].notes[note]--;
            }
            return UMUGU_SUCCESS;
        }

        case UMUGU_MIDI_CONTROL: {
            int ch = umugu_midi_event_status_channel(event.msg);
            int ctrl = umugu_midi_event_byte1(event.msg);
            int value = umugu_midi_event_byte2(event.msg);
            switch (ctrl) {
                case 0x00: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BANK_SELECT] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BANK_SELECT] |= (value << 7);
                    break;
                }

                case 0x01: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_MOD_WHEEL] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_MOD_WHEEL] |= (value << 7);
                    break;
                }

                case 0x02: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BREATH] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BREATH] |= (value << 7);
                    break;
                }

                case 0x04: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FOOT] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FOOT] |= (value << 7);
                    break;
                }

                case 0x05: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_PORTAMENTO_TIME] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_PORTAMENTO_TIME] |= (value << 7);
                    break;
                }
                case 0x06: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_DATA] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_DATA] |= (value << 7);
                    break;
                }
                case 0x07: {
                    midi->channels[ch].volume &= 0x7F;
                    midi->channels[ch].volume |= (value << 7);
                    break;
                }
                case 0x08: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BALANCE] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BALANCE] |= (value << 7);
                    break;
                }
                case 0x0A: {
                    midi->channels[ch].pan &= 0x7F;
                    midi->channels[ch].pan |= (value << 7);
                    break;
                }
                case 0x0B: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_EXPRESSION] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_EXPRESSION] |= (value << 7);
                    break;
                }
                case 0x0C: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FX1] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FX1] |= (value << 7);
                    break;
                }
                case 0x0D: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FX2] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FX2] |= (value << 7);
                    break;
                }
                case 0x10: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC1] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC1] |= (value << 7);
                    break;
                }
                case 0x11: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC2] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC2] |= (value << 7);
                    break;
                }
                case 0x12: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC3] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC3] |= (value << 7);
                    break;
                }
                case 0x13: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC4] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC4] |= (value << 7);
                    break;
                }


                case 0x20: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BANK_SELECT] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BANK_SELECT] |= value;
                    break;
                }

                case 0x21: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_MOD_WHEEL] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_MOD_WHEEL] |= value;
                    break;
                }

                case 0x22: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BREATH] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BREATH] |= value;
                    break;
                }

                case 0x24: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FOOT] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FOOT] |= value;
                    break;
                }

                case 0x25: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_PORTAMENTO_TIME] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_PORTAMENTO_TIME] |= value;
                    break;
                }
                case 0x26: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_DATA] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_DATA] |= value;
                    break;
                }
                case 0x27: {
                    midi->channels[ch].volume &= 0x3F80;
                    midi->channels[ch].volume |= value;
                    break;
                }
                case 0x28: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BALANCE] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_BALANCE] |= value;
                    break;
                }
                case 0x2A: {
                    midi->channels[ch].pan &= 0x3F80;
                    midi->channels[ch].pan |= value;
                    break;
                }
                case 0x2B: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_EXPRESSION] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_EXPRESSION] |= value;
                    break;
                }
                case 0x2C: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FX1] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FX1] |= value;
                    break;
                }
                case 0x2D: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FX2] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_FX2] |= value;
                    break;
                }
                case 0x30: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC1] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC1] |= value;
                    break;
                }
                case 0x31: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC2] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC2] |= value;
                    break;
                }
                case 0x32: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC3] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC3] |= value;
                    break;
                }
                case 0x33: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC4] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC4] |= value;
                    break;
                }

                case 0x40: {
                    midi->channels[ch].ctrl_flags &= ~UMUGU_MIDI_CTRL_FLAG_DAMPER_PEDAL;
                    if (value >> 6) {
                        midi->channels[ch].ctrl_flags |= UMUGU_MIDI_CTRL_FLAG_DAMPER_PEDAL;
                    }
                    break;
                }
                case 0x41: {
                    midi->channels[ch].ctrl_flags &= ~UMUGU_MIDI_CTRL_FLAG_PORTAMENTO;
                    if (value >> 6) {
                        midi->channels[ch].ctrl_flags |= UMUGU_MIDI_CTRL_FLAG_PORTAMENTO;
                    }
                    break;
                }
                case 0x42: {
                    midi->channels[ch].ctrl_flags &= ~UMUGU_MIDI_CTRL_FLAG_SOSTENUTO;
                    if (value >> 6) {
                        midi->channels[ch].ctrl_flags |= UMUGU_MIDI_CTRL_FLAG_SOSTENUTO;
                    }
                    break;
                }
                case 0x43: {
                    midi->channels[ch].ctrl_flags &= ~UMUGU_MIDI_CTRL_FLAG_SOFT_PEDAL;
                    if (value >> 6) {
                        midi->channels[ch].ctrl_flags |= UMUGU_MIDI_CTRL_FLAG_SOFT_PEDAL;
                    }
                    break;
                }
                case 0x44: {
                    midi->channels[ch].ctrl_flags &= ~UMUGU_MIDI_CTRL_FLAG_LEGATO_FOOTSWITCH;
                    if (value >> 6) {
                        midi->channels[ch].ctrl_flags |= UMUGU_MIDI_CTRL_FLAG_LEGATO_FOOTSWITCH;
                    }
                    break;
                }
                case 0x45: {
                    midi->channels[ch].ctrl_flags &= ~UMUGU_MIDI_CTRL_FLAG_HOLD2;
                    if (value >> 6) {
                        midi->channels[ch].ctrl_flags |= UMUGU_MIDI_CTRL_FLAG_HOLD2;
                    }
                    break;
                }

                case 0x46: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_SOUND1] = value;
                    break;
                }

                case 0x47: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_SOUND2] = value;
                    break;
                }

                case 0x48: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_SOUND3] = value;
                    break;
                }
                case 0x49: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_SOUND4] = value;
                    break;
                }
                case 0x4A: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_SOUND5] = value;
                    break;
                }
                case 0x4B: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_SOUND6] = value;
                    break;
                }
                case 0x4C: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_SOUND7] = value;
                    break;
                }
                case 0x4D: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_SOUND8] = value;
                    break;
                }
                case 0x4E: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_SOUND9] = value;
                    break;
                }
                case 0x4F: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_SOUND10] = value;
                    break;
                }

                case 0x50: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC5] = value;
                    break;
                }

                case 0x51: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC6] = value;
                    break;
                }
                case 0x52: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC7] = value;
                    break;
                }
                case 0x53: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_GENERIC8] = value;
                    break;
                }
                case 0x54: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_PORTAMENTO_NOTE] = value;
                    break;
                }

                case 0x5B: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_DEPTH_FX1] = value;
                    break;
                }
                case 0x5C: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_DEPTH_FX2] = value;
                    break;
                }
                case 0x5D: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_DEPTH_FX3] = value;
                    break;
                }
                case 0x5E: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_DEPTH_FX4] = value;
                    break;
                }
                case 0x5F: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_DEPTH_FX5] = value;
                    break;
                }
                case 0x62: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_UNREG_PARAM_NUM] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_UNREG_PARAM_NUM] |= value;
                    break;
                }
                case 0x63: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_UNREG_PARAM_NUM] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_UNREG_PARAM_NUM] |= (value << 7);
                    break;
                }

                case 0x64: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_REG_PARAM_NUM] &= 0x3F80;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_REG_PARAM_NUM] |= value;
                    break;
                }
                case 0x65: {
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_REG_PARAM_NUM] &= 0x7F;
                    midi->channels[ch].ctrl[UMUGU_MIDI_CTRL_REG_PARAM_NUM] |= (value << 7);
                    break;
                }
                
                case 0x78: { /* All sound off. */
                    assert(value == 0 && "MIDI spec");
                    memset(midi->channels[ch].notes, 0, sizeof(midi->channels[ch].notes));
                    break;
                }

                case 0x79: { /* Reset all controllers. */
                    assert(value == 0 && "MIDI spec");
                    memset(midi->channels[ch].ctrl, 0, sizeof(midi->channels[ch].ctrl));
                    midi->channels[ch].ctrl_flags = 0;
                    break;
                }

                case 0x7A: { /* Local control on/off. */
                    assert(((value == 0) || (value == 127)) && "MIDI spec");
                    /* TODO(MIDI) */
                    break;
                }

                case 0x7B: { /* All notes off. */
                    assert(value == 0 && "MIDI spec");
                    memset(midi->channels[ch].notes, 0, sizeof(midi->channels[ch].notes));
                    break;
                }

                case 0x7C: { /* Omni mode off + all notes off. */
                    assert(value == 0 && "MIDI spec");
                    midi->mode = UMUGU_MIDI_MODE_POLY;
                    memset(midi->channels[ch].notes, 0, sizeof(midi->channels[ch].notes));
                    break;
                }

                case 0x7D: { /* Omni mode on + all notes off. */
                    assert(value == 0 && "MIDI spec");
                    midi->mode = UMUGU_MIDI_MODE_OMNI;
                    memset(midi->channels[ch].notes, 0, sizeof(midi->channels[ch].notes));
                    break;
                }

                case 0x7E: { /* Poly mode on/off + all notes off. */
                    /* TODO(MIDI) */
                    memset(midi->channels[ch].notes, 0, sizeof(midi->channels[ch].notes));
                    break;
                }

                case 0x7F: { /* Poly mode on + all notes off. */
                    assert(value == 0 && "MIDI spec");
                    midi->mode = UMUGU_MIDI_MODE_POLY;
                    memset(midi->channels[ch].notes, 0, sizeof(midi->channels[ch].notes));
                    break;
                }
            }
            midi->channels[ch].ctrl[ctrl] = value;
            return UMUGU_SUCCESS;
        }

        case UMUGU_MIDI_PITCH_WHEEL: {
            int ch = umugu_midi_event_status_channel(event.msg);
            int least = umugu_midi_event_byte1(event.msg);
            int most = umugu_midi_event_byte2(event.msg);
            int pitch = (least | (most << 7)) & 0x3FFF;
            midi->channels[ch].pitch = pitch - UMUGU_MIDI_PITCH_WHEEL_CENTER;
            return UMUGU_SUCCESS;
        }

        case UMUGU_MIDI_SYS: {
            switch (umugu_midi_event_byte0(event.msg)) {
                case UMUGU_MIDI_SYS_SONGPOS: {
                    int least = umugu_midi_event_byte1(event.msg);
                    int most = umugu_midi_event_byte2(event.msg);
                    midi->song_curr_beat = (least | (most << 7)) & 0x3FFF;
                    break;
                }

                case UMUGU_MIDI_SYS_SONGSEL: {
                    midi->song = umugu_midi_event_byte1(event.msg);
                    break;
                }

                case UMUGU_MIDI_SYS_TUNE: {
                    midi->is_tune_requested = 1;
                    break;
                }

                default: {
                    umugu_midi_process_byte(umugu_midi_event_byte0(event.msg));
                    umugu_midi_process_byte(umugu_midi_event_byte1(event.msg));
                    umugu_midi_process_byte(umugu_midi_event_byte2(event.msg));
                    umugu_midi_process_byte(umugu_midi_event_byte3(event.msg));
                    break;
                }
            }

            return UMUGU_SUCCESS;
        }

        default: {
            umugu_midi_process_byte(umugu_midi_event_byte0(event.msg));
            umugu_midi_process_byte(umugu_midi_event_byte1(event.msg));
            umugu_midi_process_byte(umugu_midi_event_byte2(event.msg));
            umugu_midi_process_byte(umugu_midi_event_byte3(event.msg));
            return UMUGU_SUCCESS;
        }
    }
}
