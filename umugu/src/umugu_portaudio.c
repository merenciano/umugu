#include "umugu.h"
#include "umugu_internal.h"
#include <portaudio.h>
#include <string.h>

#define umugu__pa_check_err()       \
    if (umugu__pa_err != paNoError) \
    umugu__pa_terminate()

/* static PaStreamParameters umugu__pa_input_params; */
static PaStreamParameters umugu__pa_output_params;
static PaStream* umugu__pa_stream;
static PaError umugu__pa_err;

static void umugu__pa_terminate(void)
{
    umugu_ctx *ctx = umugu_get_context();
    ctx->log("Error PortAudio: %s\n", Pa_GetErrorText(umugu__pa_err));
    Pa_Terminate();
    ctx->log("An error occurred while using the portaudio stream.\n"
             "\tError number: %d.\n"
             "\tError message: %s.",
             umugu__pa_err, Pa_GetErrorText(umugu__pa_err));
    ctx->abort();
}

static int umugu__pa_callback(const void *in, void *out,
    unsigned long fpb, const PaStreamCallbackTimeInfo *timeinfo,
    PaStreamCallbackFlags flags, void *data)
{
    um__unused((void*)in), um__unused((void*)timeinfo), um__unused(&flags);
    umugu_pipeline *graph = (umugu_pipeline*)data;
    umugu_signal sig = {.count = fpb};
    umugu_node *it = graph->root;
    umugu_node_call(UMUGU_FN_GETSIGNAL, &it, &sig);
    umugu_sample* w = sig.samples;
    umugu_sample* o = (umugu_sample*)out;
    while (fpb--) {
        *o++ = *w++;
    }
    return paContinue;
}

void umugu_start_stream(void)
{
    umugu__pa_err = Pa_StartStream(umugu__pa_stream);
    umugu__pa_check_err();
}

void umugu_stop_stream(void)
{
    umugu__pa_err = Pa_StopStream(umugu__pa_stream);
    umugu__pa_check_err();
}

void umugu_init_backend(void)
{
    umugu__pa_err = Pa_Initialize();
    umugu__pa_check_err();

    umugu__pa_output_params.device = Pa_GetDefaultOutputDevice();
    if (umugu__pa_output_params.device == paNoDevice) {
        umugu__pa_terminate();
    }

    umugu__pa_output_params.channelCount = 2;
    umugu__pa_output_params.sampleFormat = paFloat32;
    umugu__pa_output_params.suggestedLatency = Pa_GetDeviceInfo(umugu__pa_output_params.device)->defaultLowOutputLatency;
    umugu__pa_output_params.hostApiSpecificStreamInfo = NULL;

    umugu__pa_err = Pa_OpenStream(&umugu__pa_stream, NULL, &umugu__pa_output_params,
        UMUGU_SAMPLE_RATE, paFramesPerBufferUnspecified, paClipOff, umugu__pa_callback, &umugu_get_context()->pipeline);

    umugu__pa_check_err();
}

void umugu_close_backend(void)
{
    umugu__pa_err = Pa_CloseStream(umugu__pa_stream);
    umugu__pa_check_err();
    Pa_Terminate();
}
