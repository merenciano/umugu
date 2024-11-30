/*
                               u   mu         gu
    "Venga, a ver si te buscas una musiquilla guapa, ¿no colega?" - El Pirri.


    DOC:
        - Signals:
            - ctx->io.out_audio:

    TODO:
      Library code:
        - Resampler: sample rate conversions.
        - Stream abstraction:
            - File i/o
            - Ring buffer
        - Finish swapping the remaining stdio.h calls with the umugu_io ones
        - Evolve the fatal_err callback into critical_err, which is like the actual fatal,
           but has extra data to allow a last attempt to handle the error without closing the app.
           Some examples would be to provide fallbacks of some essential resource, confirm that
           a controller is properly connected then retry, provide a larger memory arena or
           to configure another audio backend.
        - Extend the log func for loglevel, file, func and line.
        - Persistent alloc with the exact size for node infos and pipeline nodes.
        - Revisit the optional deps like PortAudio. The current header-only with optional impl
           does not work well with development tools.
        - Move the builtin nodes to umugu.c

      Project:
        - Sanitizers build config.
        - Fuzzy tests.
        - Lib documentation.
        - Gather tracing and metrics data if UMUGU_TRACE is defined
        - Verbose logs, core dump, stack trace print...
        - Benchmark and unit test of: timer, names, hash table, math...

    Maybe:
        - Embed minimal fallbacks of the required assets in the library.
        - Preprocessor directives for optional improvements, like better timer, generics or
           constexpr.
        - Language bindings (python first).
        - CI setup for the project.
        - More audio formats, backends, controllers...
        - Web and Android ports

*/

#ifndef __UMUGU_H__
#define __UMUGU_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UMUGU_VERSION 900
#define UMUGU_VERSION_STRING "0.9.1"

#ifndef UMUGU_API
#define UMUGU_API
#endif

#define UMUGU_NAME_LEN 32
#define UMUGU_PATH_LEN 64
#define UMUGU_NOTE_COUNT 128

/* TODO: Remove this. Use permanent allocations from the arena instead. */
#define UMUGU_DEFAULT_NODE_INFO_CAPACITY 64
#define UMUGU_FALLBACK_PIPELINE_CAPACITY 8
#define UMUGU_MIXER_MAX_INPUTS 8

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  CORE UTILITIES
 */

/* Text struct for name handling. The whole array is taken into account
   for string operations like comparison or hashing so care must be taken
   to keep them zeroed from strlen to UMUGU_NAME_LEN. */
typedef struct {
    char str[UMUGU_NAME_LEN];
} umugu_name;

/* FORWARD DECL */
typedef struct umugu_config umugu_config;
typedef struct umugu_ctx umugu_ctx;
typedef struct umugu_io umugu_io;
typedef struct umugu_samples umugu_samples;
typedef struct umugu_signal umugu_signal;
typedef struct umugu_attrib_info umugu_attrib_info;
typedef struct umugu_node_type_info umugu_node_type_info;
typedef struct umugu_node umugu_node;
typedef struct umugu_pipeline umugu_pipeline;

typedef int umugu_state;             /* enum umugu_state_ */
typedef int umugu_waveform;          /* enum umugu_waveform_ */
typedef uint16_t umugu_type;         /* enum umugu_type_ */
typedef uint32_t umugu_fn_flags;     /* enum umugu_fn_flags_ */
typedef uint32_t umugu_attrib_flags; /* enum umugu_attrib_flags_ */

typedef int (*umugu_node_func)(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags);

/* PUBLIC API */
UMUGU_API umugu_ctx *umugu_load(const umugu_config *cfg);
UMUGU_API void umugu_unload(umugu_ctx *ctx);
UMUGU_API int umugu_process(umugu_ctx *ctx, size_t frames);

UMUGU_API int umugu_pipeline_export(umugu_ctx *ctx, const char *filename);
UMUGU_API int umugu_pipeline_import(umugu_ctx *ctx, const char *filename);

/* DATA TYPES */

enum {
    UMUGU_BADIDX = -1, /* Invalid state for variables used as index/offset. */
    UMUGU_NONE = 0,
    UMUGU_SUCCESS,
    UMUGU_NOOP, /* Not an error since the expected result was already met. */

    UMUGU_ERR = -2000,
    UMUGU_ERR_NULL,
    UMUGU_ERR_ARGS,
    UMUGU_ERR_MEM,
    UMUGU_ERR_FILE,
    UMUGU_ERR_CONFIG,
    UMUGU_ERR_PLUG,
    UMUGU_ERR_STREAM,
    UMUGU_ERR_AUDIO_BACKEND,
    UMUGU_ERR_MIDI,
    UMUGU_ERR_FULL_STORAGE,
    UMUGU_ERR_FULL_TABLE,
};

/**
 * Wave shapes and noises that can be calculated on demand by the oscillator.
 * This library does not embed lookup tables for them anymore.
 */
enum umugu_waveform_ {
    UMUGU_WAVEFORM_SINE = 0,
    UMUGU_WAVEFORM_SAWSIN,
    UMUGU_WAVEFORM_SAW,
    UMUGU_WAVEFORM_TRIANGLE,
    UMUGU_WAVEFORM_SQUARE,
    UMUGU_WAVEFORM_WHITE_NOISE,
    UMUGU_WAVEFORM_COUNT
};

/**
 * Data type identifiers for defining the signal format and the node and attribs metadata.
 */
enum umugu_type_ {
    UMUGU_TYPE_VOID = 0,
    UMUGU_TYPE_FLOAT,
    UMUGU_TYPE_DOUBLE,
    UMUGU_TYPE_INT8,
    UMUGU_TYPE_INT16,
    UMUGU_TYPE_INT32,
    UMUGU_TYPE_INT64,
    UMUGU_TYPE_UINT8,
    UMUGU_TYPE_UINT16,
    UMUGU_TYPE_UINT32,
    UMUGU_TYPE_UINT64,
    UMUGU_TYPE_TEXT,
    UMUGU_TYPE_BOOL,

    /* Keep the bit as the last enum entry to be able to refer to bit N adding N to
     * its numeric value without colliding with another enum entry. */
    UMUGU_TYPE_BIT,
    UMUGU_TYPE_COUNT
};

enum umugu_fn_flags_ {
    UMUGU_NOFLAG = 0,
    UMUGU_FN_INIT_DEFAULTS = 0x1,
};

/**
 * Additional attribute descriptors. Used as hints for manipulating opaque node data.
 * @note It is common to develop the ui tools not knowing which nodes will be plugged-in.
 */
enum umugu_attrib_flags_ {
    UMUGU_ATTR_FLAG_NONE = 0,
    UMUGU_ATTR_RDONLY = 0x1,
    UMUGU_ATTR_RANGE = 0x2,
    UMUGU_ATTR_PLOTLINE = 0x4,
    UMUGU_ATTR_DEBUG = 0x8,
};

/* Node field descriptor with type metadata for external node communication
 * in a generic manner. The objective is to be able to serialize, interact
 * and draw widgets to interact with unknown nodes as long as they have
 * defined the metadata of the public variables. */
struct umugu_attrib_info {
    umugu_name name;
    uint16_t offset_bytes; /* from this struct's start */
    umugu_type type;
    int count; /* =1 for single variables and >1 for representing array sizes */
    umugu_attrib_flags flags;
    union {
        struct {
            float min;
            float max;
        } rangef;
        struct {
            int min; /* TODO: Get rid of this union if it is not used for anything else. */
            int max;
        } rangei;
    } misc;
};

/**
 * @brief Node metadata.
 * Like @ref umugu_attrib_info does for single node attributes, this struct
 * provides the required type info for opaque nodes manipulation. */
struct umugu_node_type_info {
    umugu_name name;
    int size_bytes;
    int attrib_count;
    umugu_node_func (*getfn)(int fn);
    const umugu_attrib_info *attribs;
    void *plug_handle;
};

struct umugu_samples {
    float *samples;
    int frame_count;
    int8_t channel_count;
};

struct umugu_signal {
    umugu_samples samples;
    bool interleaved_channels;
    umugu_type format;
    int sample_rate;
};

struct umugu_node {
    umugu_samples out_pipe;
    int8_t input_channel;
    uint16_t prev_node; /* Index in umugu_ctx->pipeline->nodes. */
    uint16_t info_idx;  /* Index in umugu_ctx->node_info. */
};

/* Defines the audio fx pipeline graph. Pipelines are the scenes of umugu.
 * This is where the data that define the output sound is stored. It contains
 * the graph defining the node i/o relations between them and its data.
 * Is the struct that has to be imported/exported or generated with tools
 * like umg-editor. */
struct umugu_pipeline {
    umugu_node *nodes[64];
    int64_t node_count;
    umugu_signal sig; // Internal signal config.
    // TODO: Add in and out signals here.
};

/**
 * @brief Initial configuration provided by the user at context creation.
 *  No instances of this struct are kept in memory once the lib is loaded.
 */
struct umugu_config {
    /* Add loglevel and extra info (file, pretty func, line)*/
    int (*log_fn)(const char *fmt, ...);
    /* TODO: Improve this callback providing opportunities for error handling by te user.  */
    void (*fatal_err_fn)(int err, const char *msg, const char *file, int line);
    /* This func should return the size of the file just in case the buffer was not big enough */
    size_t (*load_file_fn)(const char *filename, void *buffer, size_t buf_size);

    const char *config_file;
    void *arena;
    size_t arena_size;

    /**
     * Simplest pipeline possible. It will be used as a fallback if the pipeline file cannot
     * be loaded.
     */
    umugu_name fallback_ppln[UMUGU_FALLBACK_PIPELINE_CAPACITY];
    size_t fallback_ppln_node_count;
};

/* Input and output abstraction.
 * Communication layer between umugu and platform implementations. */
struct umugu_io {
    int (*log)(const char *fmt, ...);
    size_t (*file_read)(const char *filename, void *buffer, size_t buf_size);
    void (*fatal)(int err, const char *msg, const char *file, int line);

    umugu_signal in_audio;

    /* The primary output of the library. Its contents (if any) are the next
     * samples to be played by the audio backend. This signal is the result
     * of the last umugu_produce_signal call.
     * WRITE: Umugu. */
    umugu_signal out_audio;

    const char *backend_name;
    void *backend_data;

    int (*backend_init)(umugu_ctx *ctx);
    int (*backend_read)(umugu_ctx *ctx, int frames);
    int (*backend_write)(umugu_ctx *ctx, int frames);
    void (*backend_shutdown)(umugu_ctx *ctx);
};

/**
 * Context's FSM states. They are used mostly for tracing and
 * checks (not strictly required for functionality).
 */
enum umugu_state_ {
    UMUGU_STATE_INVALID = 0,
    UMUGU_STATE_LOADING,
    UMUGU_STATE_IDLE,
    UMUGU_STATE_PROCESSING,
    UMUGU_STATE_UNLOADING
};

struct umugu_ctx {
    umugu_state state;
    umugu_io io;             /* Input / output abstraction layer. */
    umugu_pipeline pipeline; /* Audio processing pipeline. */

    /* Nodes type info. */
    umugu_node_type_info nodes_info[UMUGU_DEFAULT_NODE_INFO_CAPACITY];
    int32_t nodes_info_next;

    /* Memory arena. */
    uint8_t *arena_head;      /* First byte of the memory arena. */
    ptrdiff_t arena_capacity; /* In bytes. */
    uint8_t *arena_pers_end;  /* First byte after the permanent allocated region. */
    uint8_t *arena_tail;      /* First unallocated byte of the arena. */

    /* Debug metric data. */
    int64_t ppln_iterations;   // Counter that increases for each umugu_produce_signal call.
    int64_t ppln_it_allocated; // Number of arena bytes allocated this pipeline iteration.

    int64_t init_time_ns;         // initialization time in nanoseconds.
    int64_t ppln_process_time_ns; // last pipeline process time in nanoseconds.

    /* Default values. */
    char fallback_wav_file[UMUGU_PATH_LEN];
    char fallback_soundfont2_file[UMUGU_PATH_LEN];
    char fallback_midi_device[UMUGU_PATH_LEN];
};

#ifdef __cplusplus
}
#endif

#endif /* __UMUGU_H__ */
