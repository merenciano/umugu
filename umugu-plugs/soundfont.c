#include <umugu/umugu.h>

#include <assert.h>
#include <fluidsynth.h>
#include <string.h>

typedef struct {
    umugu_node node;
    fluid_settings_t *settings;
    fluid_synth_t *synth;
    fluid_midi_driver_t *midi;
    fluid_audio_driver_t *audio;
    char soundfont_file[UMUGU_PATH_LEN];
} um_sf;

static inline int um_init(umugu_node *node) {
    umugu_ctx *ctx = umugu_get_context();
    um_sf *self = (void *)node;
    self->settings = NULL;
    self->synth = NULL;
    self->midi = NULL;
    self->audio = NULL;

    strncpy(self->soundfont_file, ctx->config.default_audio_file,
            UMUGU_PATH_LEN);
    node->out_pipe.channels = 2;

    self->settings = new_fluid_settings();
    if (!self->settings) {
        ctx->io.log("FluidSynth error: settings creation.\n");
        return UMUGU_ERR;
    }

    fluid_settings_setnum(self->settings, "synth.sample-rate",
                          ctx->config.sample_rate);
    fluid_settings_setstr(self->settings, "midi.driver", "alsa_raw");
    fluid_settings_setstr(self->settings, "midi.alsa.device",
                          ctx->config.default_midi_device);

    self->synth = new_fluid_synth(self->settings);
    if (!self->synth) {
        ctx->io.log("FluidSynth error: synth creation.\n");
        umugu_node_dispatch(node, UMUGU_FN_RELEASE);
        return UMUGU_ERR;
    }

    if (fluid_synth_sfload(self->synth, self->soundfont_file, 1) == -1) {
        ctx->io.log("FluidSynth error: soundfont2 file loading (%s).\n",
                    self->soundfont_file);
        umugu_node_dispatch(node, UMUGU_FN_RELEASE);
        return UMUGU_ERR;
    }

    self->midi = new_fluid_midi_driver(
        self->settings, fluid_synth_handle_midi_event, self->synth);

    if (!self->midi) {
        ctx->io.log("FluidSynth error: midi driver (%s) creation.\n",
                    ctx->config.default_audio_file);
        umugu_node_dispatch(node, UMUGU_FN_RELEASE);
        return UMUGU_ERR;
    }

    return UMUGU_SUCCESS;
}

int um_defaults(umugu_node *node) {
    um_sf *self = (void *)node;
    strncpy(self->soundfont_file,
            umugu_get_context()->config.default_audio_file, UMUGU_PATH_LEN);
    return UMUGU_SUCCESS;
}

int um_process(umugu_node *node) {
    um_sf *self = (void *)node;
    umugu_ctx *ctx = umugu_get_context();
    if (ctx->pipeline_iteration <= node->iteration) {
        ctx->io.log("SoundFont node NOOP in process().\n");
        return UMUGU_NOOP;
    }

    umugu_signal *pip = &node->out_pipe;
    umugu_sample *out = umugu_alloc_signal(pip);
    memset(out, 0, sizeof(*out) * pip->count * pip->channels);
    assert(pip->channels == 2);
    umugu_sample *fx[2], *dry[2];
    fx[0] = out;
    dry[0] = out;
    fx[1] = out + node->out_pipe.count;
    dry[1] = out + node->out_pipe.count;

    if (fluid_synth_process(self->synth, node->out_pipe.count, 2, fx, 2, dry) ==
        FLUID_FAILED) {
        ctx->io.log("FluidSynth: SynthProcess failed.\n");
        return UMUGU_ERR;
    }

    node->iteration = ctx->pipeline_iteration;
    return UMUGU_SUCCESS;
}

int um_release(umugu_node *node) {
    um_sf *self = (void *)node;
    if (self->audio) {
        delete_fluid_audio_driver(self->audio);
    }

    if (self->midi) {
        delete_fluid_midi_driver(self->midi);
    }

    if (self->synth) {
        delete_fluid_synth(self->synth);
    }

    if (self->settings) {
        delete_fluid_settings(self->settings);
    }

    self->settings = NULL;
    self->synth = NULL;
    self->midi = NULL;
    self->audio = NULL;

    return UMUGU_SUCCESS;
}

umugu_node_fn getfn(int fn) {
    switch (fn) {
        case UMUGU_FN_INIT:
            return um_init;
        case UMUGU_FN_DEFAULTS:
            return um_defaults;
        case UMUGU_FN_PROCESS:
            return um_process;
        case UMUGU_FN_RELEASE:
            return um_release;
        default:
            return NULL;
    }
}

static const umugu_var_info metadata[] = {
    {.name = {.str = "SoundFontFilename"},
     .offset_bytes = offsetof(um_sf, soundfont_file),
     .type = UMUGU_TYPE_TEXT,
     .count = UMUGU_PATH_LEN,
     .flags = 0}};

const size_t size = sizeof(um_sf);
const umugu_var_info *vars = &metadata[0];
const int32_t var_count = sizeof(metadata) / sizeof(*metadata);
