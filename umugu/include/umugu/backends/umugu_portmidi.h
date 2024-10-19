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
#include <stdio.h>

#define UMUGU_MIDI_IN_BUFSIZE 127
#define UMUGU_MIDI_OUT_BUFSIZE 127
#define UMUGU_TIME_PROC ((PmTimeProcPtr)Pt_Time)

static PmStream *um__pm_stream;
static PmDeviceID um__pm_dev;
static void *um__pm_input_driver_info;

int umugu_midi_init(void) {
    Pt_Start(1, NULL, NULL);
    Pm_Initialize();

    for (int i = 0; i < Pm_CountDevices(); ++i) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (info->input && !info->opened) {
            printf("MIDI: Selected input device: %s.\n", info->name);
            um__pm_dev = i;
            break;
        }
    }
}

int umugu_midi_start_stream(void) {
    Pm_OpenInput(&um__pm_stream, um__pm_dev, um__pm_input_driver_info,
                 UMUGU_MIDI_IN_BUFSIZE, UMUGU_TIME_PROC, NULL);
}

#endif /* UMUGU_PORTMIDI_IMPL */
