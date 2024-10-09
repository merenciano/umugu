#ifndef __UMUGU_PORTAUDIO_19_H__
#define __UMUGU_PORTAUDIO_19_H__

/* Generic interface. */

#ifdef __cplusplus
extern "C" {
#endif
int umugu_audio_backend_init(void);
int umugu_audio_backend_close(void);
int umugu_audio_backend_start_stream(void);
int umugu_audio_backend_stop_stream(void);
#ifdef __cplusplus
}
#endif

#endif /* __UMUGU_PORTAUDIO_19_H__ */

#ifdef UMUGU_PORTAUDIO19_IMPL

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
    umugu_ctx *ctx = (umugu_ctx *)data;
    ctx->io.time_margin_sec =
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

    ctx->io.out_audio_signal.frames = (umugu_frame *)out_buffer;
    ctx->io.out_audio_signal.count = frame_count;
    ctx->io.out_audio_signal.rate = UMUGU_SAMPLE_RATE;
    ctx->io.out_audio_signal.type = UMUGU_SAMPLE_TYPE;
    ctx->io.out_audio_signal.channels = UMUGU_CHANNELS;
    ctx->io.out_audio_signal.capacity = frame_count;

    umugu_produce_signal();

    if (!ctx->io.audio_backend_ready) {
        ctx->log("Umugu's context audio output has closed suddenly!\n"
                 "Aborting the current PortAudio stream.\n");
        ctx->io.audio_backend_stream_running = 0;
        return paAbort;
    }

    if (!ctx->io.audio_backend_stream_running) {
        ctx->log("Umugu's context audio output has stopped running!\n"
                 "Setting the current PortAudio stream as completed.\n");
        return paComplete;
    }

    return paContinue;
}

int umugu_audio_backend_start_stream(void) {
    umugu_ctx *ctx = umugu_get_context();
    if (ctx->io.audio_backend_stream_running) {
        ctx->log("The output stream is already running.\n"
                 "Ignoring this umugu_start_stream() call.\n");
        return UMUGU_NOOP;
    }

    um__pa_err = Pa_StartStream(um__pa_stream);
    if (um__pa_err != paNoError) {
        um__pa_terminate();
        ctx->log("Error PortAudio: Unable to start the stream.\n");
        return UMUGU_ERR_STREAM;
    }

    ctx->io.audio_backend_stream_running = 1;
    return UMUGU_SUCCESS;
}

int umugu_audio_backend_stop_stream(void) {
    umugu_ctx *ctx = umugu_get_context();
    if (!ctx->io.audio_backend_stream_running) {
        ctx->log("Trying to stop a stopped stream.\n"
                 "Ignoring this umugu_stop_stream() call.\n");
        return UMUGU_NOOP;
    }

    um__pa_err = Pa_StopStream(um__pa_stream);
    if (um__pa_err != paNoError) {
        um__pa_terminate();
        ctx->log("Error PortAudio: Unable to stop the stream.\n");
        return UMUGU_ERR_STREAM;
    }

    ctx->io.audio_backend_stream_running = 0;
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

int umugu_audio_backend_init(void) {
    umugu_ctx *ctx = umugu_get_context();
    if (ctx->io.audio_backend_ready) {
        ctx->log("Umugu has already been initialized and the audio backend is "
                 "loaded.\n"
                 "Ignoring this umugu_init() call...\n");
        return UMUGU_NOOP;
    }

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
        ctx->log(
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
                               paClipOff, um__pa_callback, ctx);

    if (um__pa_err != paNoError) {
        um__pa_terminate();
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    ctx->io.backend_handle = um__pa_stream;
    ctx->io.audio_backend_ready = 1;
    return UMUGU_SUCCESS;
}

int umugu_audio_backend_close(void) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->io.backend_handle = NULL;
    ctx->io.audio_backend_stream_running = 0;
    ctx->io.audio_backend_ready = 0;

    um__pa_err = Pa_CloseStream(um__pa_stream);
    if (um__pa_err != paNoError) {
        um__pa_terminate();
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    Pa_Terminate();
    return UMUGU_SUCCESS;
}

#endif /* UMUGU_PORTAUDIO19_IMPL */
