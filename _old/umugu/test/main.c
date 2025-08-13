#include "../src/debugu.h"

#include <umugu/umugu.h>
#include <umugu/umugu_internal.h>

#define UMUGU_PORTAUDIO19_IMPL
#include <umugu/backends/umugu_portaudio19.h>

#define UMUGU_STDOUT_IMPL
#include <umugu/backends/umugu_stdout.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int
um_print(const char *fmt, ...)
{
#ifdef UMUGU_SILENT
    return 1;
#endif
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}

static void
fatal_error(int err, const char *msg, const char *file, int line)
{
    printf("Umugu fatal error in %s (%d): Code %d - %s.\n", file, line, err, msg);
    __builtin_trap();
}

static size_t
file_load(const char *filename, void *buffer, size_t buf_size)
{
    UM_TRACE_ZONE();
    FILE *fp = fopen(filename, "r");
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    if ((size + 1) > buf_size) {
        fclose(fp);
        return size + 1;
    }

    fseek(fp, 0, SEEK_SET);
    fread(buffer, 1, size, fp);
    ((char *)buffer)[size] = '\0';
    fclose(fp);
    return size + 1;
}

static inline void
app_print_help(void)
{
    printf("\tplumugu - testing utilities for libumugu.\n");
    printf("Usage: plumugu -Xvalue  #Dash followed by capital letter followed by value)\n");
    printf("Opts:\n");
    printf("\t-Cfpath \t\tConfig file, can be combined with other options.\n");
    printf("\t-T      \t\tUnit test pass, can be combined with other options.\n");
    printf("\t-Sdevice\t\tSynth + midi controller, needs a valid midi device name.\n");
    printf("\t-Pfpath \t\tPlayback of the specified file using an audio backend.\n");
    printf("\t-Ofpath \t\tOutputs audio signal to stdout. Can be piped into a music player.\n");
    printf("\t\t\t\t       e.g. plumugu -Olittlewing.wav | aplay\n");
    printf("\nExample: load config, run tests and generate audio signal from midi events.\n");
    printf("\t\t\t\tplumugu -C../configs/synth.ucg -T -Sminilab3\n");
}

static void
app_print_vector(const char *title, um_complex *x, int n)
{
    printf("%s (dim=%d):", title, n);
    for (int i = 0; i < n; i++) {
        printf(" %5.2f,%5.2f ", x[i].real, x[i].imag);
    }
    printf("\n");
}

static inline void
app_run_unit_test(void)
{
    UM_TRACE_ZONE();
    static const int N = 1 << 3; /* N-point FFT, iFFT */
    um_complex v[N], v1[N], scratch[N];
    int k;

    /* Fill v[] with a function of known FFT: */
    for (k = 0; k < N; k++) {
        v[k].real = 0.125 * cos(2 * M_PI * k / (double)N);
        v[k].imag = 0.125 * sin(2 * M_PI * k / (double)N);
        v1[k].real = 0.3 * cos(2 * M_PI * k / (double)N);
        v1[k].imag = -0.3 * sin(2 * M_PI * k / (double)N);
    }

    /* FFT, iFFT of v[]: */
    app_print_vector("Orig", v, N);
    um_fft(v, N, scratch);
    app_print_vector(" FFT", v, N);
    um_ifft(v, N, scratch);
    app_print_vector("iFFT", v, N);

    /* FFT, iFFT of v1[]: */
    app_print_vector("Orig", v1, N);
    um_fft(v1, N, scratch);
    app_print_vector(" FFT", v1, N);
    um_ifft(v1, N, scratch);
    app_print_vector("iFFT", v1, N);
}

static inline void
app_stdout_demo(umugu_ctx *ctx)
{
    UM_TRACE_ZONE();
    umugu_audio_backend_play(ctx, 60000);
    getchar();
}

static inline void
app_playback_demo(umugu_ctx *ctx)
{
    UM_TRACE_ZONE();
    umugu_audio_backend_init(ctx);
    umugu_audio_backend_start_stream(ctx);

    printf("Blocking main thread (audio is being processed in another thread via "
           "callbacks)\nPress any key and 'Enter' for closing...\n");
    getchar();

    umugu_audio_backend_stop_stream(ctx);
}

static inline void
app_midi_synth_demo(umugu_ctx *ctx)
{
    UM_TRACE_ZONE();
    umugu_audio_backend_init(ctx);
    umugu_audio_backend_start_stream(ctx);

    printf("Blocking main thread (audio is being processed in another thread via "
           "callbacks)\nPress any key and 'Enter' for closing...\n");
    getchar();

    umugu_audio_backend_stop_stream(ctx);
}

static inline const umugu_attrib_info *
app_find_node_attrib(umugu_ctx *ctx, const umugu_node *node, umugu_name attr_name)
{
    const size_t attrib_count = ctx->nodes_info[node->info_idx].attrib_count;
    for (size_t i = 0; i < attrib_count; ++i) {
        const umugu_attrib_info *attrib = &ctx->nodes_info[node->info_idx].attribs[i];
        if (!memcmp(attrib->name.str, attr_name.str, UMUGU_NAME_LEN)) {
            return attrib;
        }
    }
    return NULL;
}

static inline void
app_set_node_filepath(umugu_ctx *ctx, const char *file, umugu_name attr_name)
{
    umugu_node *n = ctx->pipeline.nodes[0];
    const umugu_attrib_info *attri = app_find_node_attrib(ctx, n, attr_name);
    strncpy((char *)n + attri->offset_bytes, file, UMUGU_PATH_LEN);
}

enum { APP_ARENA_SIZE = 1024 * 1024 };
static char g_arena[APP_ARENA_SIZE];

int
main(int argc, char **argv)
{
    UM_TRACE_ZONE();
    enum {
        APP_NONE = 0,
        APP_STDOUT, /* no backend */
        APP_MIDI_SYNTH,
        APP_PLAYBACK,
    } mode = argc == 1 ? APP_MIDI_SYNTH : APP_NONE;

    bool print_help = false;
    bool run_tests = false;
    const char *arg_filename = NULL;
    const char *arg_midi_device = "hw:Minilab3";

    umugu_config umgcfg = {
        .config_file = "../assets/config.ucg",
        .arena = g_arena,
        .arena_size = APP_ARENA_SIZE,
        .fallback_ppln = {{"SoundFont"}, {"Output"}},
        .fallback_ppln_node_count = 2,
        .log_fn = um_print,
        .fatal_err_fn = fatal_error,
        .load_file_fn = file_load};

    for (int i = 1; i < argc; ++i) {
        if (*argv[i] != '-') {
            print_help = true;
            continue;
        }

        switch (argv[i][1]) {
        case 'O': {
            mode = APP_STDOUT;
            umgcfg.fallback_ppln[0] = (umugu_name){"WavFilePlayer"};
            arg_filename = &argv[i][2];
            break;
        }
        case 'P': {
            mode = APP_PLAYBACK;
            umgcfg.fallback_ppln[0] = (umugu_name){"WavFilePlayer"};
            arg_filename = &argv[i][2];
            break;
        }
        case 'S': {
            mode = APP_MIDI_SYNTH;
            umgcfg.fallback_ppln[0] = (umugu_name){"SoundFont"};
            arg_midi_device = &argv[i][2];
            break;
        }
        case 'C': {
            umgcfg.config_file = &argv[i][2];
            break;
        }
        case 'T': {
            run_tests = true;
            break;
        }
        }
    }

    if (print_help || (mode == APP_NONE && !run_tests)) {
        app_print_help();
        return 1;
    }

    /* umugu loading */
    umugu_ctx *umgctx = umugu_load(&umgcfg);

    if (run_tests) {
        app_run_unit_test();
    }

    switch (mode) {
    case APP_MIDI_SYNTH: {
        app_set_node_filepath(umgctx, arg_midi_device, (umugu_name){"MidiDeviceName"});
        app_midi_synth_demo(umgctx);
        break;
    }
    case APP_STDOUT: {
        app_set_node_filepath(umgctx, arg_filename, (umugu_name){"Filename"});
        app_stdout_demo(umgctx);
        break;
    }
    case APP_PLAYBACK: {
        app_set_node_filepath(umgctx, arg_filename, (umugu_name){"Filename"});
        app_playback_demo(umgctx);
        break;
    }
    default:
        break;
    }

    /* print debug info */
    umugu_mem_arena_print(umgctx);
    umugu_pipeline_print(umgctx);
    umugu_out_signal_print(umgctx);

    /* umugu unloading */
    umugu_unload(umgctx);

    return 0;
}
