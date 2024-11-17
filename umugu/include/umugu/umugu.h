/*
                               u   mu         gu
    "Venga, a ver si te buscas una musiquilla guapa, ¿no colega?" - El Pirri.

    TODO:
        - stdbool.h
        - Rename in symbols: var -> attribute
        - Simple and performant hash for umugu_name to be used as a key in
          associative containers (although I'm quite sure that linear search
          over contiguous data using 1 memcmp per node is probably faster with
          the few elements that this library usually has).
        - Sanitize build config.
        - Profiling build config.
        - Doxygen.
        - Sample rate conversions.
        - Write config file parser (probably .ini)
*/

#ifndef __UMUGU_H__
#define __UMUGU_H__

#include <stddef.h>
#include <stdint.h>

/*
    BUILD OPTIONS
    Keeping them here in the header for convenience and documentation
    but should be defined by the build system.
*/
/* #define UMUGU_DISABLE_MIDI */
// #define UMUGU_DEBUG
// #define UMUGU_TRACE
// #define UMUGU_VERBOSE

/* INTERNAL DEFS. Changing them can break things. */
#define UMUGU_NAME_LEN 32

/* I would like to make it configurable in case larger paths are needed, but
 * changing it breaks ABI compatibility without changing version. */
#define UMUGU_PATH_LEN 64

/* TODO: Remove this. Use permanent allocations from the arena instead. */
#define UMUGU_DEFAULT_NODE_INFO_CAPACITY 64
#define UMUGU_NOTE_COUNT 128
#define UMUGU_DRUM_COUNT 47
#define UMUGU_MIXER_MAX_INPUTS 8

#define UMUGU_VERSION_MAJOR 0
#define UMUGU_VERSION_MINOR 9
#define UMUGU_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct umugu_ctx umugu_ctx;

enum {
    /** Invalid state for variables used as index/offset. */
    UMUGU_BADIDX = -1,
    /** Generic unspecified value. */
    UMUGU_NONE = 0,

    /** Obtained from funcions that have returned along the normal path. */
    UMUGU_SUCCESS,

    /** No operation. Usually returned by redundant calls.
     * Not an error since the expected result was already met. */
    UMUGU_NOOP,

    UMUGU_ERR_LAST = -32768,
    UMUGU_ERR_ARGS,
    UMUGU_ERR_MEM,
    UMUGU_ERR_FILE,
    UMUGU_ERR_PLUG,
    UMUGU_ERR_STREAM,
    UMUGU_ERR_AUDIO_BACKEND,
    UMUGU_ERR_MIDI,
    UMUGU_ERR_NULL,
    UMUGU_ERR,
};

/* Default sample type for the audio pipeline. This type will be used for
 * representing the amplitude (wave) values of an audio signal.
 * In umugu_signal and other data structs is represented as UMUGU_TYPE_FLOAT. */
typedef float umugu_sample;

enum {
    UMUGU_SIGNAL_ENABLED = 0x0001,
    UMUGU_SIGNAL_INTERLEAVED = 0x0002,
};

/**
 * @brief Audio signal with fixed format for internal pipeline processing.
 * Float32 sample format.
 * Non interleaved channels. The amount varies according to the node type.
 * Sample rate is 48000 by default (@see umugu_config.sample_rate).
 * @note For I/O audio configurable signals @see umugu_generic_signal.
 */
typedef struct umugu_signal {
    /**
     * Buffer for storing the sample values of the signal.
     * Expected buffer capacity calculation:
     * @code{.c}
     * size_t Capacity = frames * channels * sizeof(umugu_sample);
     * @endcode
     */
    umugu_sample *samples;
    int32_t count;    /** Number of sample values per channel, i.e. frames. */
    int32_t channels; /** Number of channels in the signal. */
} umugu_signal;

/** Generic audio signal. It can be a whole song or a small chunk for the output
 *  stream, or a buffer for visual debugging by plotting its frames.
 *  This struct also contains all the required data to play the audio
 *  correctly or convert the signal to another format.
 *  @note The memory layout is compatible with umugu_signal in case it is
 *  convenient to cast between pointers.
 */
typedef struct umugu_generic_signal {
    void *sample_data;
    int32_t count;    /* number of samples per channel (frames) in the signal */
    int32_t channels; /* number of channels, e.g. 2 for stereo */
    int32_t rate;     /* samples per sec */
    int16_t type;     /* sample data type, UMUGU_TYPE_ */
    uint16_t flags;   /* config and status, UMUGU_SIGNAL_  */
} umugu_generic_signal;

enum {
    UMUGU_WAVE_SINE = 0,
    UMUGU_WAVE_SAWSIN,
    UMUGU_WAVE_SAW,
    UMUGU_WAVE_TRIANGLE,
    UMUGU_WAVE_SQUARE,
    UMUGU_WAVE_WHITE_NOISE,
    UMUGU_WAVE_COUNT
};

/* Node virtual functions. */
enum { UMUGU_FN_INIT, UMUGU_FN_DEFAULTS, UMUGU_FN_PROCESS, UMUGU_FN_RELEASE };
typedef int umugu_fn;

/* Text struct for name handling. The whole array is taken into account
   for string operations like comparison or hashing so care must be taken
   to keep them zeroed from strlen to UMUGU_NAME_LEN. */
typedef struct {
    char str[UMUGU_NAME_LEN];
} umugu_name;

enum umugu_types_e {
    UMUGU_TYPE_BIT0 = 0,
    UMUGU_TYPE_BIT1,
    UMUGU_TYPE_BIT2,
    UMUGU_TYPE_BIT3,
    UMUGU_TYPE_BIT4,
    UMUGU_TYPE_BIT5,
    UMUGU_TYPE_BIT6,
    UMUGU_TYPE_BIT7,
    /* Bit positions until 63 can be specified with UMUGU_TYPE_BIT0 + N. */
    UMUGU_TYPE_UINT8 = 64,
    UMUGU_TYPE_INT16,
    UMUGU_TYPE_INT32,
    UMUGU_TYPE_FLOAT,
    UMUGU_TYPE_INT64,
    UMUGU_TYPE_INT8,
    UMUGU_TYPE_UINT16,
    UMUGU_TYPE_UINT32,
    UMUGU_TYPE_UINT64,
    UMUGU_TYPE_DOUBLE,
    UMUGU_TYPE_TEXT,
    UMUGU_TYPE_BOOL,
    UMUGU_TYPE_COUNT
};

enum umugu_var_flags_e {
    UMUGU_VAR_NONE = 0,
    UMUGU_VAR_RDONLY = 1,
    UMUGU_VAR_RANGE = 1 << 1,
    UMUGU_VAR_PLOTLINE = 1 << 2,
    UMUGU_VAR_DEBUG = 1 << 3,
};

/* Node field descriptor with type metadata for external node communication
 * in a generic manner. The objective is to be able to serialize, interact
 * and draw widgets to interact with unknown nodes as long as they have
 * defined the metadata of the public variables. */
typedef struct {
    umugu_name name;
    int16_t offset_bytes; /* from struct's start */
    int16_t type;         /* enum umugu_types_e */
    int32_t count;        /* number of elements i.e. array */
    int32_t flags;
    union {
        struct {
            float min;
            float max;
        } rangef;
        struct {
            int32_t min;
            int32_t max;
        } rangei;
    } misc;
} umugu_var_info;

/* Node base class. All the created nodes should have this type as the first
 * data member of the struct. The name is enough for obtaining its associated
 * metadata (see: umugu_node_info) and then the offsets to the interface. */
typedef struct umugu_node {
    umugu_signal out_pipe;
    uint8_t type; /* Index in node_info */
    int8_t input_channel;
    uint16_t prev_node; /* Index in pipeline of the previous node. */
    uint32_t iteration;
} umugu_node;

typedef int (*umugu_node_fn)(umugu_node *);

/* Like umugu_var_info does for data fields or variables, this struct
 * provides the required type information for interacting with nodes without
 * having access to its struct or having to provide specific implementations
 * for them. */
typedef struct {
    umugu_name name;
    int32_t size_bytes;
    int32_t var_count;
    umugu_node_fn (*getfn)(int fn);
    const umugu_var_info *vars; /* field metadata */
    void *plug_handle;
} umugu_node_info;

enum umugu_ctrl_e {
    UMUGU_CTRL_FX1,
    UMUGU_CTRL_FX2,
    UMUGU_CTRL_5,
    UMUGU_CTRL_6,
    UMUGU_CTRL_1,
    UMUGU_CTRL_2,
    UMUGU_CTRL_3,
    UMUGU_CTRL_4,
    UMUGU_CTRL_VARIATION,
    UMUGU_CTRL_TIMBRE,
    UMUGU_CTRL_RELEASE_TIME,
    UMUGU_CTRL_ATTACK_TIME,
    UMUGU_CTRL_BRIGHTNESS,
    UMUGU_CTRL_SOUND_6,
    UMUGU_CTRL_SOUND_7,
    UMUGU_CTRL_SOUND_8,
    UMUGU_CTRL_VOLUME,
    UMUGU_CTRL_PITCHWHEEL,
    UMUGU_CTRL_CHORUS_DEPTH,
    UMUGU_CTRL_COUNT
};

typedef struct {
    int8_t notes[UMUGU_NOTE_COUNT];
    int8_t drums[UMUGU_DRUM_COUNT];
    int8_t values[UMUGU_CTRL_COUNT];
    int32_t device_idx;
    char device_name[UMUGU_PATH_LEN];

    void *data_stream;
} umugu_ctrl;

/* Defines the audio fx pipeline graph. Pipelines are the scenes of umugu.
 * This is where the data that define the output sound is stored. It contains
 * the graph defining the node i/o relations between them and its data.
 * Is the struct that has to be imported/exported or generated with tools
 * like umg-editor. */
typedef struct {
    umugu_node *nodes[64];
    int64_t node_count;
} umugu_pipeline;

enum umugu_events_e {
    UMUGU_EVENT_INIT_FINISHED,
    UMUGU_EVENT_PIPELINE_PROCESS_STARTED,
    UMUGU_EVENT_PIPELINE_PROCESS_FINISHED,
    UMUGU_EVENT_CONTRO
};

/* Input and output abstraction.
 * Communication layer between umugu and platform implementations. */
typedef struct {
    int (*log)(const char *fmt, ...);
    void (*event_listener)(umugu_ctx *ctx, int event);

    umugu_ctrl controller;
    umugu_generic_signal in_audio;

    /* The primary output of the library. Its contents (if any) are the next
     * samples to be played by the audio backend. This signal is the result
     * of the last umugu_produce_signal call.
     * WRITE: Umugu. */
    umugu_generic_signal out_audio;

    void *backend_data;
    void *user_data;
} umugu_io;

typedef struct {
    uint8_t *buffer;
    uint8_t *temp_next;
    uint8_t *pers_next;
    ptrdiff_t capacity; /* In bytes. */
    ptrdiff_t alloc_counter;
} umugu_mem_arena;

/* Assigns a memory chunk to the context's arena.
 * This lib does not take ownership of the buffer, but it must remain valid
 * until another one is assigned or program termination.
 */
int umugu_set_arena(void *buffer, size_t bytes);

/**
 * Allocates the desired amount of bytes from the arena. The allocated buffer
 * can not be released and will be valid until the execution ends. More
 * permanent memory allocated means less memory available for the temporal
 * memory buffer. This allocation function should be used at initialization
 * time, since any permanent allocation done after starting allocating temporal
 * memory will overwrite the temporal buffer space without taking any care of
 * the stored data.
 * @param bytes Desired allocation size in bytes.
 * @return Pointer to the start of the allocated buffer.
 */
void *umugu_alloc_pers(size_t bytes);

/**
 * Allocated the desired amount of bytes from the arena. The allocated buffer
 * will be 'released' automatically when subsequent temporal allocations fill
 * the capacity of the temporal memory buffer and the allocated memory gets
 * overwritten. If that happens before the overwritten data gets obsolete, it
 * is considered a bug since a bigger buffer sould have been assigned to the
 * arena, @see umugu_set_arena.
 * @note Think about the temporary memory as a circular (or ring) buffer
 * that uses the remaining memory of the arena after the permanent allocations,
 * i.e. arena_capacity - persistent_allocations.
 * @param bytes Desired allocation size in bytes.
 * @return Pointer to the start of the allocated buffer.
 */
void *umugu_alloc_temp(size_t bytes);

/**
 * Allocates a large enough buffer for the signal depending on the output audio
 * signal of the umugu_io.
 * @param signal Pointer to a signal lvalue owned by the caller. The allocated
 * buffer will be assigned to that signal's sample_data.
 * @return A convenience pointer to the signal's allocated sample buffer. It is
 * the same as signal->sample_data after this call.
 */
umugu_sample *umugu_alloc_signal(umugu_signal *signal);
void *umugu_alloc_generic_signal(umugu_generic_signal *signal);

enum umugu_cfg_flags_e {
    UMUGU_CONFIG_VERBOSE, /* Enable verbose logs. */
    UMUGU_CONFIG_DEBUG,   /* Enable runtime checks and asserts. */
    UMUGU_CONFIG_TRACE,   /* Enable trace report generation. */

    /* Automatic scan and connection of midi controllers at startup. */
    UMUGU_CONFIG_SCAN_MIDI_AUTO,

    /* Enable controller emulation with keyboard. */
    UMUGU_CONFIG_KEYBOARD_EMU,
};

typedef struct {
    uint64_t flags;
    char default_pipeline_file[UMUGU_PATH_LEN];
    char default_audio_file[UMUGU_PATH_LEN];
    char default_midi_device[UMUGU_PATH_LEN];
    umugu_name fallback_pipeline_nodes[8];
    int32_t sample_rate;
} umugu_config;

/**
 * @brief Identifiers to represent every possible state in which the context
 * may be in.
 * The states are used for the implementation of a basic state machine, created
 * for debugging purposes (since it's pretty useless for release builds).
 * The aim of the states is to delimit the different stages of execution, which
 * allows for writing debug checks and provides insight on traces and logs.
 */
typedef enum umugu_state {

    /**
     * @brief The @ref umugu_ctx exists but has not been initialized yet.
     * This state is valid, as well as useless until @ref umugu_init is called.
     * Trying to perform any other action will bring problems since being in
     * this state means that there is no configuration, node infos, pipeline,
     * memory arena, even the @ref umugu_io::log function is probably missing.
     * Once the context starts the initialization, there is no way back to this
     * state.
     */
    UMUGU_STATE_UNINITIALIZED = 0,

    /**
     * @brief Time between the call to @ref umugu_init and returning from
     * pipeline loading.
     * This state time is spent setting default values, parsing the config,
     * loading the pipeline and pairing external controllers.
     * @note Audio backend initializations and other external dependencies do
     * not affect this state (only when their streams are bound to umugu,
     * which triggers the transition from @ref UMUGU_STATE_READY to
     * @ref UMUGU_STATE_LISTENING).
     */
    UMUGU_STATE_INITIALIZING,

    /**
     * Blocked until some task is assigned.
     * This usually happens when umugu
     * finishes its initialization but no stream has been opened for audio
     * processing. User intervention is required in order to leave this
     * state (usually starting a realtime stream or requesting audio
     * playback).
     * @note From now on, no more persistent memory allocations should be made
     * in the arena, instead, the remaining space will be used as a ring
     * buffer offering temporary allocations for frame processing. These
     * allocations can not be released either, but it must be taken into account
     * that the buffer must have enough capacity for all the necessary
     * allocations within a frame. Once the processing is finished, it does not
     * matter if that memory region gets overwritten in the next frame, since it
     * is no longer needed (the frame where it was relevant is already playing
     * through the speakers).
     * @note There is no technical impediment to keep allocating permament
     * memory, but there are no good reasons to do so. However, that should be
     * avoided during pipeline processing because the persistent memory region
     * will grow by taking up space in the ring buffer, regardless of whether
     * that memory region has just been allocated and is currently in use.
     */
    UMUGU_STATE_READY,

    /**
     * Blocked, waiting between periodic callbacks or ring buffer locks.
     * This state implies that there are real-time processes running but
     * there are no buffers left to process this frame.
     * It is expected that the state will continuously alternate between this
     * and @ref UMUGU_STATE_REALTIME_PROCESSING.
     */
    UMUGU_STATE_LISTENING,

    /** Busy producing frame audio (iterating the pipeline).
     *  It is common for the context to constantly alternate between this state
     *  and @ref UMUGU_STATE_LISTENING while a low latency stream is open,
     *  since the audio is generated in small batches with high frequency.
     */
    UMUGU_STATE_REALTIME_PROCESSING,

    /**
     * @brief Busy processing whole files, performing conversions or dumping
     * data into a file.
     * The context will remain in this state until the processing is complete
     * or aborted. Then the context returns to @ref UMUGU_STATE_READY or, in
     * most cases, the program ends since that was its only task.
     */
    UMUGU_STATE_NON_REALTIME_PROCESSING,

    /**
     * @brief This state lasts for the duration of @ref umugu_close.
     * There is no coming back to any active state once the shutdown is started.
     * @note Even if you, as a developer, see no point in destroying nicely
     * every instance allocated when closing the program (I would certainly
     * rather have the virtual memory manager blow up the entire page in one
     * fell swoop than wait 4s for everything to be carefully destroyed for no
     * good reason at all) even then, I strongly recommend destroying umugu and
     * the audio backend properly, since they use system resources like audio
     * buffers, streams and devices, which may remaing blocked without a proper
     * program shutdown, preventing its usage until the affected service
     * is reloaded or the system rebooted (all bullshit tho, I'm always closing
     * with ^C and never had any problem).
     */
    UMUGU_STATE_SHUTDOWN,

    /**
     * @brief Context released successfully.
     * The instance is now useless and should be discarded. This state is
     * not uncommon for single context apps because in most cases the process
     * finishes when the shutdown returns.
     */
    UMUGU_STATE_TERMINATED,

    /**
     * @brief Panic mode. Active while dumping debug info somewhere or having
     * the execution stopped, but if the context reach this state there is
     * something to fix.
     */
    UMUGU_STATE_UNRECOVERABLE
} umugu_state;

struct umugu_ctx {
    /* Configuration variables. */
    umugu_config config;
    umugu_state state;

    /* Input / output abstraction layer. */
    umugu_io io;

    umugu_pipeline pipeline;
    umugu_mem_arena arena;

    /* Loaded node types metadata (non persistent). */
    umugu_node_info nodes_info[UMUGU_DEFAULT_NODE_INFO_CAPACITY];
    int32_t nodes_info_next;

    /** Counter that increases for each @ref umugu_produce_signal call. */
    int64_t pipeline_iteration;
    /** Duration of the configuration load in nanoseconds. */
    int64_t init_time_ns;
    /** Duration of the pipeline import or generation in nanoseconds. */
    int64_t pipeline_load_time_ns;
    /** Duration of the last controller update in nanoseconds. */
    int64_t controller_update_time_ns;
    /** Duration of the last pipeline pass in nanoseconds. */
    int64_t pipeline_update_time_ns;
};

int umugu_init(void);

/* Updates the audio output signal.
 * Populates the audio output signal with generated samples from the pipeline.
 * The caller is expected to set the output signal configuration before. */
int umugu_produce_signal(void);

/* Export the current context's pipeline to binary data.
 * If ctx param is NULL, umugu_get_context() will be used.
 * Return UMUGU_SUCCESS if saved successfuly or UMUGU_ERR_FILE if file open
 * fails. */
int umugu_export_pipeline(const char *filename, umugu_ctx *ctx);

/* Import the binary file as the current context's pipeline.
 * Return UMUGU_SUCCESS if loaded successfuly, UMUGU_ERR_FILE if can not open
 * the file or the file is not formatted as a pipeline binary, and
 * UMUGU_ERR_MEM if the allocation of the pipeline buffer fails. */
int umugu_import_pipeline(const char *filename);

/**
 * Initializes the current context's pipeline using the given names and default
 * values.
 * @param node_names Array of the desired node types.
 * @param node_count Number of names in the node_names array.
 */
int umugu_pipeline_generate(const umugu_name *node_names, int node_count);

/* Search the file lib<name>.so in the rpath and load it if found.
 * Return the index of the context's node infos array where it has been copied.
 * If the dynamic object can not be found, return UMUGU_ERR_PLUG. */
int umugu_plug(const umugu_name *name);
void umugu_unplug(umugu_node_info *info);

/* Node virtual dispatching.
 * @param fn Function identifier, valid definitions are prefixed with UMUGU_FN_.
 * Return UMUGU_SUCCESS if the call is performed and UMUGU_ERR otherwise. */
int umugu_node_dispatch(umugu_node *node, int fn);

/* Search the node name in the built-in nodes and add the node info to
 * the context. If there is no built-in node with that name, looks for
 * lib<node_name>.so in the rpath and plug it if found.
 * Return a pointer to the loaded node info in the context or NULL if not found.
 */
const umugu_node_info *umugu_node_info_load(const umugu_name *name);

/* Active context instance manipulation. */
void umugu_set_context(umugu_ctx *new_ctx);
umugu_ctx *umugu_get_context(void);

#ifdef __cplusplus
}
#endif

#endif /* __UMUGU_H__ */
