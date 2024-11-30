#include <umugu/umugu.h>
#include <umugu/umugu_internal.h>

#include <fluidsynth.h>
#include <string.h>

typedef struct {
    umugu_node node;
    fluid_settings_t *settings;
    fluid_synth_t *synth;
    fluid_midi_driver_t *midi;
    fluid_audio_driver_t *audio;
    char soundfont_file[UMUGU_PATH_LEN];
    char midi_dev_name[UMUGU_PATH_LEN];
} um_sf;

static inline int um_init(umugu_ctx *ctx, umugu_node *node,
                          umugu_fn_flags flags) {
    um_sf *self = (void *)node;
    self->settings = NULL;
    self->synth = NULL;
    self->midi = NULL;
    self->audio = NULL;

    if (self->soundfont_file[0] == 0 || (flags & UMUGU_FN_INIT_DEFAULTS)) {
        strncpy(self->soundfont_file, ctx->fallback_soundfont2_file,
                UMUGU_PATH_LEN);
    }

    if (self->midi_dev_name[0] == 0 || (flags & UMUGU_FN_INIT_DEFAULTS)) {
        strncpy(self->midi_dev_name, ctx->fallback_midi_device, UMUGU_PATH_LEN);
    }

    node->out_pipe.channel_count = 2;

    if (flags & UMUGU_FN_INIT_DEFAULTS) {
        return UMUGU_SUCCESS;
    }

    self->settings = new_fluid_settings();
    if (!self->settings) {
        ctx->io.log("FluidSynth error: settings creation.\n");
        return UMUGU_ERR;
    }

    fluid_settings_setnum(self->settings, "synth.sample-rate",
                          ctx->pipeline.sig.sample_rate);
    fluid_settings_setstr(self->settings, "midi.driver", "alsa_raw");
    fluid_settings_setstr(self->settings, "midi.alsa.device",
                          self->midi_dev_name);

    self->synth = new_fluid_synth(self->settings);
    if (!self->synth) {
        ctx->io.log("FluidSynth error: synth creation.\n");
        um_node_dispatch(ctx, node, UMUGU_FN_RELEASE, UMUGU_NOFLAG);
        return UMUGU_ERR;
    }

    if (fluid_synth_sfload(self->synth, self->soundfont_file, 1) == -1) {
        ctx->io.log("FluidSynth error: soundfont2 file loading (%s).\n",
                    self->soundfont_file);
        um_node_dispatch(ctx, node, UMUGU_FN_RELEASE, UMUGU_NOFLAG);
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    self->midi = new_fluid_midi_driver(
        self->settings, fluid_synth_handle_midi_event, self->synth);

    if (!self->midi) {
        ctx->io.log("FluidSynth error: midi driver (%s) creation.\n",
                    self->midi_dev_name);
        um_node_dispatch(ctx, node, UMUGU_FN_RELEASE, UMUGU_NOFLAG);
        return UMUGU_ERR_MIDI;
    }

    return UMUGU_SUCCESS;
}

int um_process(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags) {
    UM_UNUSED(flags);
    um_sf *self = (void *)node;
    umugu_samples *pip = &node->out_pipe;
    float *out = um_alloc_samples(ctx, pip);
    memset(out, 0, sizeof(*out) * pip->frame_count * pip->channel_count);
    UMUGU_ASSERT(pip->channel_count == 2);
    float *fx[2], *dry[2];
    fx[0] = out;
    dry[0] = out;
    fx[1] = out + node->out_pipe.frame_count;
    dry[1] = out + node->out_pipe.frame_count;

    if (fluid_synth_process(self->synth, node->out_pipe.frame_count, 2, fx, 2,
                            dry) == FLUID_FAILED) {
        ctx->io.log("FluidSynth: SynthProcess failed.\n");
        return UMUGU_ERR_AUDIO_BACKEND;
    }

    return UMUGU_SUCCESS;
}

int um_release(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags) {
    UM_UNUSED(ctx), UM_UNUSED(flags);
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

umugu_node_func getfn(int fn) {
    switch (fn) {
        case UMUGU_FN_INIT:
            return um_init;
        case UMUGU_FN_PROCESS:
            return um_process;
        case UMUGU_FN_RELEASE:
            return um_release;
        default:
            return NULL;
    }
}

static const umugu_attrib_info metadata[] = {
    {.name = {"SoundFontFilename"},
     .offset_bytes = offsetof(um_sf, soundfont_file),
     .type = UMUGU_TYPE_TEXT,
     .count = UMUGU_PATH_LEN,
     .flags = 0},
    {.name = {"MidiDeviceName"},
     .offset_bytes = offsetof(um_sf, midi_dev_name),
     .type = UMUGU_TYPE_TEXT,
     .count = UMUGU_PATH_LEN,
     .flags = 0}};

const size_t size = sizeof(um_sf);
const umugu_attrib_info *attribs = &metadata[0];
const int32_t attrib_count = sizeof(metadata) / sizeof(*metadata);
