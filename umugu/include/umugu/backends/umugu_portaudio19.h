#ifndef __UMUGU_PORTAUDIO_19_H__
#define __UMUGU_PORTAUDIO_19_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct umugu_portaudio {
    const char *name;
    int stream_running;
    int input_active;  /* Writes to context.io.in_audio. */
    int output_active; /* Reads from context.io.out_audio. */
    double time_current;
    /* Seconds between the stream callback call and the moment when the produced
     * signal will output the DAC. */
    double time_margin_sec;
    void *extra_data;
} umugu_portaudio;

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

typedef struct {
    PaStream *stream;
    PaError error;
    PaStreamParameters input_params;
    PaStreamParameters output_params;
} um__pa_internal;

static um__pa_internal um__pa_intern = {
    .stream = NULL,
    .error = paNoError,
    .input_params = {.device = paNoDevice,
                     .channelCount = 0,
                     .sampleFormat = paCustomFormat,
                     .suggestedLatency = 0.0,
                     .hostApiSpecificStreamInfo = NULL},
    .output_params = {.device = paNoDevice,
                      .channelCount = 0,
                      .sampleFormat = paCustomFormat,
                      .suggestedLatency = 0.0,
                      .hostApiSpecificStreamInfo = NULL}};

static umugu_portaudio um__pa_data = {.name = "PortAudio19",
                                      .stream_running = 0,
                                      .input_active = 0,
                                      .output_active = 0,
                                      .time_current = 0.0,
                                      .time_margin_sec = 0.0,
                                      .extra_data = &um__pa_intern};

static inline PaSampleFormat um__pa_samplefmt(int umugu_type,
                                              int non_interleaved) {
    switch (umugu_type) {
    case UMUGU_TYPE_FLOAT:
        return paFloat32 | (paNonInterleaved * non_interleaved);
    case UMUGU_TYPE_INT32:
        return paInt32 | (paNonInterleaved * non_interleaved);
    case UMUGU_TYPE_INT16:
        return paInt16 | (paNonInterleaved * non_interleaved);
    case UMUGU_TYPE_INT8:
        return paInt8 | (paNonInterleaved * non_interleaved);
    case UMUGU_TYPE_UINT8:
        return paUInt8 | (paNonInterleaved * non_interleaved);
    default:
        return paCustomFormat;
    }
}

static inline void um__pa_terminate(void) {
    umugu_ctx *ctx = umugu_get_context();
    ctx->io.log("Error PortAudio: %s\n", Pa_GetErrorText(um__pa_intern.error));
    Pa_Terminate();
    ctx->io.log("An error occurred while using the portaudio stream.\n"
                "\tError number: %d.\n"
                "\tError message: %s.",
                um__pa_intern.error, Pa_GetErrorText(um__pa_intern.error));
}

static inline int um__pa_callback(const void *in_buffer, void *out_buffer,
                                  unsigned long frame_count,
                                  const PaStreamCallbackTimeInfo *timeinfo,
                                  PaStreamCallbackFlags flags, void *data) {
    umugu_ctx *ctx = (umugu_ctx *)data;
    umugu_portaudio *pa_data = (umugu_portaudio *)ctx->io.backend_data;

    if (!pa_data->input_active && !pa_data->output_active) {
        ctx->io.log("Input and output inactive, aborting PortAudio stream.\n");
        return paAbort;
    }

    pa_data->time_current = timeinfo->currentTime;
    pa_data->time_margin_sec =
        timeinfo->outputBufferDacTime - timeinfo->currentTime;

    if (flags & paInputUnderflow) {
        ctx->io.log("PortAudio callback input underflow.\n");
    }

    if (flags & paInputOverflow) {
        ctx->io.log("PortAudio callback input overflow.\n");
    }

    if (flags & paOutputUnderflow) {
        ctx->io.log("PortAudio callback output underflow.\n");
    }

    if (flags & paOutputOverflow) {
        ctx->io.log("PortAudio callback output overflow.\n");
    }

    if (flags & paPrimingOutput) {
        ctx->io.log("PortAudio callback priming output.\n");
    }

    if (pa_data->input_active) {
        ctx->io.in_audio.sample_data = (void *)in_buffer;
        ctx->io.in_audio.count = frame_count;
    }

    if (pa_data->output_active) {
        ctx->io.out_audio.sample_data = out_buffer;
        ctx->io.out_audio.count = frame_count;
        umugu_produce_signal();
    }

    return paContinue;
}

int umugu_audio_backend_start_stream(void) {
    umugu_ctx *ctx = umugu_get_context();
    umugu_portaudio *pa_data = (umugu_portaudio *)ctx->io.backend_data;
    if (pa_data->stream_running) {
        ctx->io.log("The stream is already running. Ignoring call...\n");
        return UMUGU_NOOP;
    }

    um__pa_intern.error = Pa_StartStream(um__pa_intern.stream);
    if (um__pa_intern.error != paNoError) {
        um__pa_terminate();
        ctx->io.log("Error PortAudio: Unable to start the stream.\n");
        return UMUGU_ERR_STREAM;
    }

    pa_data->stream_running = 1;
    ctx->io.log("PortAudio: Stream running.\n");
    return UMUGU_SUCCESS;
}

int umugu_audio_backend_stop_stream(void) {
    umugu_ctx *ctx = umugu_get_context();
    umugu_portaudio *pa_data = (umugu_portaudio *)ctx->io.backend_data;
    if (!pa_data->stream_running) {
        return UMUGU_NOOP;
    }

    um__pa_intern.error = Pa_StopStream(um__pa_intern.stream);
    if (um__pa_intern.error != paNoError) {
        um__pa_terminate();
        ctx->io.log("Error PortAudio: Unable to stop the stream.\n");
        return UMUGU_ERR_STREAM;
    }

    pa_data->stream_running = 0;
    return UMUGU_SUCCESS;
}

int umugu_audio_backend_init(void) {
    umugu_ctx *ctx = umugu_get_context();
    if (ctx->io.backend_data) {
        /* This or another backend initialized. */
        ctx->io.log(
            "There is an initialized backend in current umugu_context\n");
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    ctx->io.backend_data = &um__pa_data;
    umugu_portaudio *pa_data = (umugu_portaudio *)ctx->io.backend_data;

    if (pa_data->stream_running) {
        ctx->io.log(
            "Umugu has already been initialized and the audio backend is "
            "loaded.\n"
            "Ignoring this umugu_init() call...\n");
        return UMUGU_NOOP;
    }

    um__pa_intern.error = Pa_Initialize();
    if (um__pa_intern.error != paNoError) {
        um__pa_terminate();
        ctx->io.backend_data = NULL;
        ctx->io.log("PortAudio Error: Initialize failed.\n");
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    PaStreamParameters *iparams = NULL;
    PaStreamParameters *oparams = NULL;
    um__pa_intern.input_params.device = Pa_GetDefaultInputDevice();
    um__pa_intern.output_params.device = Pa_GetDefaultOutputDevice();
    umugu_generic_signal *umugu_isig = &ctx->io.in_audio;
    umugu_generic_signal *umugu_osig = &ctx->io.out_audio;
    double sample_rate = (double)ctx->config.sample_rate;
    /* Input signal params */
    if (um__pa_intern.input_params.device != paNoDevice &&
        (umugu_isig->flags & UMUGU_SIGNAL_ENABLED)) {
        sample_rate = (double)umugu_isig->rate;
        um__pa_intern.input_params.channelCount = umugu_isig->channels;
        um__pa_intern.input_params.sampleFormat = um__pa_samplefmt(
            umugu_isig->type,
            umugu_isig->flags & UMUGU_SIGNAL_INTERLEAVED ? 0 : 1);

        if (um__pa_intern.input_params.sampleFormat == paCustomFormat) {
            ctx->io.log(
                "PortAudio input stream sample format error.\n"
                "Umugu type %d to PortAudio sample format not implemented yet, "
                "please add the corresponding switch case.\n",
                umugu_isig->type);
        } else {
            um__pa_intern.input_params.suggestedLatency =
                Pa_GetDeviceInfo(um__pa_intern.input_params.device)
                    ->defaultLowInputLatency;
            um__pa_intern.input_params.hostApiSpecificStreamInfo = NULL;
            iparams = &um__pa_intern.input_params;
        }
    }

    /* Output signal params */
    if (um__pa_intern.output_params.device != paNoDevice &&
        (umugu_osig->flags & UMUGU_SIGNAL_ENABLED)) {
        sample_rate = (double)umugu_osig->rate;
        um__pa_intern.output_params.channelCount = umugu_osig->channels;
        um__pa_intern.output_params.sampleFormat = um__pa_samplefmt(
            umugu_osig->type,
            umugu_osig->flags & UMUGU_SIGNAL_INTERLEAVED ? 0 : 1);

        if (um__pa_intern.output_params.sampleFormat == paCustomFormat) {
            ctx->io.log(
                "PortAudio output stream sample format error.\n"
                "Umugu type %d to PortAudio sample format not implemented yet, "
                "please add the corresponding switch case.\n",
                umugu_osig->type);
        } else {
            um__pa_intern.output_params.suggestedLatency =
                Pa_GetDeviceInfo(um__pa_intern.output_params.device)
                    ->defaultLowOutputLatency;
            um__pa_intern.output_params.hostApiSpecificStreamInfo = NULL;
            oparams = &um__pa_intern.output_params;
        }
    }

    um__pa_intern.error = Pa_OpenStream(
        &um__pa_intern.stream, iparams, oparams, sample_rate,
        paFramesPerBufferUnspecified, paClipOff, um__pa_callback, ctx);

    if (um__pa_intern.error != paNoError) {
        um__pa_terminate();
        ctx->io.backend_data = NULL;
        return UMUGU_ERR_AUDIO_BACKEND;
    }
    ctx->io.log("PortAudio: Stream Opened!\n");

    if (iparams) {
        if (sample_rate != (double)umugu_isig->rate) {
            ctx->io.log("Input and Output sample rates are not equal. "
                        "Using %d from audio output for both.\n",
                        umugu_osig->rate);
            umugu_isig->rate = umugu_osig->rate;
            pa_data->input_active = 1;
        }
    }

    if (oparams) {
        pa_data->output_active = 1;
    }

    return UMUGU_SUCCESS;
}

int umugu_audio_backend_close(void) {
    umugu_ctx *ctx = umugu_get_context();

    um__pa_intern.error = Pa_CloseStream(um__pa_intern.stream);
    if (um__pa_intern.error != paNoError) {
        um__pa_terminate();
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    ctx->io.backend_data = NULL;
    Pa_Terminate();
    return UMUGU_SUCCESS;
}

#endif /* UMUGU_PORTAUDIO19_IMPL */
