#ifndef __PORTAUDIO_IMPL_H__
#define __PORTAUDIO_IMPL_H__
#include "../umugu_internal.h"

#include <umugu/umugu.h>

#include <portaudio.h>

static PaStreamParameters um__pa_output_params;
static PaStream *um__pa_stream;
static PaError um__pa_err;

static inline void um__pa_terminate(void) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->log("Error PortAudio: %s\n", Pa_GetErrorText(um__pa_err));
    Pa_Terminate();
    ctx->log("An error occurred while using the portaudio stream.\n"
             "\tError number: %d.\n"
             "\tError message: %s.",
             um__pa_err, Pa_GetErrorText(um__pa_err));
}

static inline int um__pa_callback(const void *in_buffer, void *out_buffer,
                                  unsigned long frame_count,
                                  const PaStreamCallbackTimeInfo *timeinfo,
                                  PaStreamCallbackFlags flags, void *data) {
    (void)in_buffer;
    umugu_ctx *ctx = data;
    ctx->audio_output.time_margin_sec =
        timeinfo->outputBufferDacTime - timeinfo->currentTime;

    if (flags & paOutputUnderflow) {
        ctx->log("PortAudio callback output underflow.\n");
    }

    if (flags & paOutputOverflow) {
        ctx->log("PortAudio callback output overflow.\n");
    }

    if (flags & paPrimingOutput) {
        ctx->log("PortAudio callback priming output.\n");
    }

    umugu_pipeline *graph = &ctx->pipeline;
    ctx->audio_output.output = (umugu_signal){.frames = out_buffer,
                                              .count = frame_count,
                                              .rate = UMUGU_SAMPLE_RATE,
                                              .type = UMUGU_SAMPLE_TYPE,
                                              .channels = UMUGU_CHANNELS,
                                              .capacity = frame_count};

    /* TODO: Use umugu_produce_signal(). */
    umugu_node *it = graph->root;
    umugu_node_call(UMUGU_FN_GETSIGNAL, &it, &(ctx->audio_output.output));

    if (!ctx->audio_output.open) {
        ctx->log("Umugu's context audio output has closed suddenly!\n"
                 "Aborting the current PortAudio stream.\n");
        ctx->audio_output.running = 0;
        return paAbort;
    }

    if (!ctx->audio_output.running) {
        ctx->log("Umugu's context audio output has stopped running!\n"
                 "Setting the current PortAudio stream as completed.\n");
        return paComplete;
    }

    return paContinue;
}

int um__backend_start_stream(void) {
    um__pa_err = Pa_StartStream(um__pa_stream);
    if (um__pa_err != paNoError) {
        um__pa_terminate();
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    return UMUGU_SUCCESS;
}

int um__backend_stop_stream(void) {
    um__pa_err = Pa_StopStream(um__pa_stream);
    if (um__pa_err != paNoError) {
        um__pa_terminate();
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    return UMUGU_SUCCESS;
}

static inline int um__pa_sample_fmt(int umugu_type) {
    switch (umugu_type) {
    case UMUGU_TYPE_FLOAT:
        return paFloat32;
    case UMUGU_TYPE_INT32:
        return paInt32;
    case UMUGU_TYPE_INT16:
        return paInt16;
    case UMUGU_TYPE_UINT8:
        return paUInt8;
    default:
        return UMUGU_ERR_AUDIO_BACKEND;
    }
}

int um__backend_init(void) {
    um__pa_err = Pa_Initialize();
    if (um__pa_err != paNoError) {
        um__pa_terminate();
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    um__pa_output_params.device = Pa_GetDefaultOutputDevice();
    if (um__pa_output_params.device == paNoDevice) {
        um__pa_terminate();
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    int fmt = um__pa_sample_fmt(UMUGU_SAMPLE_TYPE);
    if (fmt == UMUGU_ERR_AUDIO_BACKEND) {
        umugu_get_context()->log(
            "Error initializing the audio backend (PortAudio): Conversion from "
            "umugu type %d to PortAudio sample format not implemented yet, "
            "please add the corresponding switch case.\n"
            "... Defaulting to Float32.\n",
            UMUGU_SAMPLE_TYPE);
        fmt = paFloat32;
    }

    um__pa_output_params.channelCount = UMUGU_CHANNELS;
    um__pa_output_params.sampleFormat = fmt;
    um__pa_output_params.suggestedLatency =
        Pa_GetDeviceInfo(um__pa_output_params.device)->defaultLowOutputLatency;
    um__pa_output_params.hostApiSpecificStreamInfo = NULL;

    um__pa_err = Pa_OpenStream(&um__pa_stream, NULL, &um__pa_output_params,
                               UMUGU_SAMPLE_RATE, paFramesPerBufferUnspecified,
                               paClipOff, um__pa_callback, umugu_get_context());

    if (um__pa_err != paNoError) {
        um__pa_terminate();
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    umugu_get_context()->audio_output.handle = um__pa_stream;
    return UMUGU_SUCCESS;
}

int um__backend_close(void) {
    umugu_get_context()->audio_output.handle = NULL;
    um__pa_err = Pa_CloseStream(um__pa_stream);
    if (um__pa_err != paNoError) {
        um__pa_terminate();
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    Pa_Terminate();
    return UMUGU_SUCCESS;
}

#endif // __PORTAUDIO_IMPL_H__
