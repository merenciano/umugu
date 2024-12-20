#ifndef __UMUGU_PORTAUDIO_19_H__
#define __UMUGU_PORTAUDIO_19_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct umugu_portaudio {
    double time_current;
    /* Seconds between the stream callback call and the moment when the produced
     * signal will output the DAC. */
    double time_margin_sec;
} umugu_portaudio;

struct umugu_ctx;
int umugu_audio_backend_init(struct umugu_ctx *ctx);
int umugu_audio_backend_close(struct umugu_ctx *ctx);
int umugu_audio_backend_start_stream(struct umugu_ctx *ctx);
int umugu_audio_backend_stop_stream(struct umugu_ctx *ctx);

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
    .input_params =
        {.device = paNoDevice,
         .channelCount = 0,
         .sampleFormat = paCustomFormat,
         .suggestedLatency = 0.0,
         .hostApiSpecificStreamInfo = NULL},
    .output_params = {
        .device = paNoDevice,
        .channelCount = 0,
        .sampleFormat = paCustomFormat,
        .suggestedLatency = 0.0,
        .hostApiSpecificStreamInfo = NULL}};

static umugu_portaudio um__pa_data = {.time_current = 0.0, .time_margin_sec = 0.0};

static inline PaSampleFormat
um__pa_samplefmt(int umugu_type, bool non_interleaved)
{
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

static inline void
um__pa_terminate(umugu_ctx *ctx)
{
    ctx->io.log("Error PortAudio: %s\n", Pa_GetErrorText(um__pa_intern.error));
    Pa_Terminate();
    ctx->io.log(
        "An error occurred while using the portaudio stream.\n"
        "\tError number: %d.\n"
        "\tError message: %s.",
        um__pa_intern.error, Pa_GetErrorText(um__pa_intern.error));
    ctx->io.backend_data = NULL;
    ctx->io.backend_name = NULL;
}

static inline int
um__pa_callback(
    const void *in_buffer, void *out_buffer, unsigned long frame_count,
    const PaStreamCallbackTimeInfo *timeinfo, PaStreamCallbackFlags flags, void *data)
{
    umugu_ctx *ctx = (umugu_ctx *)data;

    ((umugu_portaudio *)ctx->io.backend_data)->time_current = timeinfo->currentTime;
    ((umugu_portaudio *)ctx->io.backend_data)->time_margin_sec =
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

    ctx->io.in_audio.samples.samples = (void *)in_buffer;
    ctx->io.in_audio.samples.frame_count = in_buffer ? frame_count : 0;

    ctx->io.out_audio.samples.samples = (void *)out_buffer;
    ctx->io.out_audio.samples.frame_count = out_buffer ? frame_count : 0;

    umugu_process(ctx, frame_count);

    return paContinue;
}

int
umugu_audio_backend_start_stream(umugu_ctx *ctx)
{
    if (Pa_IsStreamActive(um__pa_intern.stream)) {
        ctx->io.log("The stream is already running. Ignoring call...\n");
        return UMUGU_NOOP;
    }

    um__pa_intern.error = Pa_StartStream(um__pa_intern.stream);
    if (um__pa_intern.error != paNoError) {
        um__pa_terminate(ctx);
        ctx->io.log("Error PortAudio: Unable to start the stream.\n");
        return UMUGU_ERR_STREAM;
    }

    ctx->io.log("PortAudio: Stream running.\n");
    return UMUGU_SUCCESS;
}

int
umugu_audio_backend_stop_stream(umugu_ctx *ctx)
{
    um__pa_intern.error = Pa_StopStream(um__pa_intern.stream);
    if (um__pa_intern.error != paNoError) {
        um__pa_terminate(ctx);
        ctx->io.log("Error PortAudio: Unable to stop the stream.\n");
        return UMUGU_ERR_STREAM;
    }

    return UMUGU_SUCCESS;
}

int
umugu_audio_backend_init(umugu_ctx *ctx)
{
    if (ctx->io.backend_name) {
        /* This or another backend initialized. */
        ctx->io.log(
            "There is an initialized backend named (%s) in current umugu_context\n",
            ctx->io.backend_name);
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    um__pa_intern.error = Pa_Initialize();
    if (um__pa_intern.error != paNoError) {
        um__pa_terminate(ctx);
        ctx->io.log("PortAudio Error: Initialize failed.\n");
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    ctx->io.backend_data = &um__pa_data;
    ctx->io.backend_name = "PortAudio19";
    PaStreamParameters *iparams = NULL;
    PaStreamParameters *oparams = NULL;
    um__pa_intern.input_params.device = Pa_GetDefaultInputDevice();
    um__pa_intern.output_params.device = Pa_GetDefaultOutputDevice();
    /* Input signal params */
    if (um__pa_intern.input_params.device != paNoDevice && ctx->io.in_audio.samples.channel_count) {
        um__pa_intern.input_params.sampleFormat = (double)ctx->io.in_audio.sample_rate;
        um__pa_samplefmt(ctx->io.in_audio.format, !ctx->io.in_audio.interleaved_channels);

        if (um__pa_intern.input_params.sampleFormat == paCustomFormat) {
            ctx->io.log(
                "PortAudio input stream sample format error.\n"
                "Umugu type %d to PortAudio sample format not implemented yet, "
                "please add the corresponding switch case.\n",
                ctx->io.in_audio.format);
        } else {
            um__pa_intern.input_params.suggestedLatency =
                Pa_GetDeviceInfo(um__pa_intern.input_params.device)->defaultLowInputLatency;
            um__pa_intern.input_params.hostApiSpecificStreamInfo = NULL;
            iparams = &um__pa_intern.input_params;
        }
    }

    /* Output signal params */
    if (um__pa_intern.output_params.device != paNoDevice &&
        ctx->io.out_audio.samples.channel_count) {
        um__pa_intern.output_params.channelCount = ctx->io.out_audio.samples.channel_count;
        um__pa_intern.output_params.sampleFormat =
            um__pa_samplefmt(ctx->io.out_audio.format, !ctx->io.out_audio.interleaved_channels);

        if (um__pa_intern.output_params.sampleFormat == paCustomFormat) {
            ctx->io.log(
                "PortAudio output stream sample format error.\n"
                "Umugu type %d to PortAudio sample format not implemented yet, "
                "please add the corresponding switch case.\n",
                ctx->io.out_audio.format);
        } else {
            um__pa_intern.output_params.suggestedLatency =
                Pa_GetDeviceInfo(um__pa_intern.output_params.device)->defaultLowOutputLatency;
            um__pa_intern.output_params.hostApiSpecificStreamInfo = NULL;
            oparams = &um__pa_intern.output_params;
        }
    }

    um__pa_intern.error = Pa_OpenStream(
        &um__pa_intern.stream, iparams, oparams, (double)ctx->io.out_audio.sample_rate,
        paFramesPerBufferUnspecified, paClipOff, um__pa_callback, ctx);

    if (um__pa_intern.error != paNoError) {
        um__pa_terminate(ctx);
        return UMUGU_ERR_AUDIO_BACKEND;
    }
    ctx->io.log("PortAudio: Stream Opened!\n");

    return UMUGU_SUCCESS;
}

int
umugu_audio_backend_close(umugu_ctx *ctx)
{
    um__pa_intern.error = Pa_CloseStream(um__pa_intern.stream);
    if (um__pa_intern.error != paNoError) {
        um__pa_terminate(ctx);
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    ctx->io.backend_name = NULL;
    ctx->io.backend_data = NULL;
    Pa_Terminate();
    return UMUGU_SUCCESS;
}

#endif /* UMUGU_PORTAUDIO19_IMPL */
