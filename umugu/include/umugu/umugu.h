/*
                               u   mu         gu
    "Venga, a ver si te buscas una musiquilla guapa, ¿no colega?" - El Pirri.

    The audio pipeline handles signals with this config:
      - Channels = 2
      - Sample rate = 48000
      - Data type = Float32

    The audio sources that read files can open other configs, but the
     signal gets converted to that config before leaving the source node.
    The same happens for output. It can have other configs but all the pipeline
     will be processed with the default signal config until the end, then
     the signal will be converted to the desired output format.

    TODO(Err)
    TODO(qol)
    TODO:
        - Implement umugu_produce_signal.
        - Make it work using int16 or even uint8, instead of float32. I want
          this to work on a ESP32.
        - Simple and performant hash for umugu_name to be used as a key in
          associative containers (although I'm quite sure that linear search
          over contiguous data using 1 memcmp per node is probably faster with
          the few elements that this library usually has).
        - Conversions between signal configurations.
        - Sanitize build config.
        - Profiling build config.
        - Doxygen.
        - Adapt to any wav config (mono, stereo, samples per sec, data size..)
        - File players for compressed formats.
        - Think about separating node config/parameter data (the one that
          controllers and UI modifies) from internal state (the one that
          the signal processing thread modifies). It's nice to hace all the
          node data together but I would like to have all the configurable
          data of the pipeline grouped for better serialization (I'm importing
          and exporting the whole chunk of pipeline data anyway but half of it
          is useless state). Or maybe I should go all-in and serialize the
          whole application state in order to make snapshots of any given
          moment for debug/inspection... or mmap the whole context to a binary
          file for easy autosaves.. or just memset and mask all the state and
          padding data and feed it to any runtime compression algorithm.

        - Organize all the config and decide how it's done. File? Defines?
        - Change pointers with arena offsets in serialized structs (I want .bin
   compat between 32-64 bit systems).
        - Get rid of heap allocations.

        - Move the audio backend implementation (portaudio19) to umg-player
   (abstract libumugu.a from audio backends)

    Symbol prefix:
        - Public interface: umugu_
        - Private impl:     um__
*/

#ifndef __UMUGU_H__
#define __UMUGU_H__

#include <stddef.h>
#include <stdint.h>

#define UMUGU_VERSION_MAJOR 0
#define UMUGU_VERSION_MINOR 2
#define UMUGU_VERSION_PATCH 0

#define UMUGU_SAMPLE_RATE 48000
#define UMUGU_SAMPLE_TYPE UMUGU_TYPE_FLOAT
#define UMUGU_CHANNELS 2

#define UMUGU_NAME_LEN 32
#define UMUGU_PATH_LEN 64

#define UMUGU_DEFAULT_SAMPLE_CAPACITY 1024
#define UMUGU_DEFAULT_NODE_INFO_CAPACITY 64

#ifdef __cplusplus
extern "C" {
#endif

enum {
    /* Generic unspecified value. */
    UMUGU_NONE = 0,

    /* Obtained from funcions that have returned along the normal path. */
    UMUGU_SUCCESS,

    /* No operation. Usually returned by redundant calls.
     * Not an error since the expected result was already met. */
    UMUGU_NOOP,

    UMUGU_ERR_LAST = -32768,
    UMUGU_ERR_ARGS,
    UMUGU_ERR_MEM,
    UMUGU_ERR_FILE,
    UMUGU_ERR_PLUG,
    UMUGU_ERR_STREAM,
    UMUGU_ERR_AUDIO_BACKEND,
    UMUGU_ERR,
};

/* Default sample type for the audio pipeline. This type will be used for
 * representing the amplitude (wave) values of an audio signal.
 * In umugu_signal and other data structs is represented as UMUGU_TYPE_FLOAT. */
typedef float umugu_sample;

/* This struct represent the state of a signal at one specific point in time,
 * i.e. the sample value of each active channel.
 */
typedef struct {
    /* TODO: Should I care for multi-channel? If the answer
       is 'yes', fix it as soon as possible. */
    umugu_sample left;
    umugu_sample right; /* TODO: umugu_sample[UMUGU_CHANNELS]; */
} umugu_frame;

/* Audio signal. It can be a whole song or a small chunk for the output
 * stream, or a buffer for visual debugging by plotting its frames.
 * This struct also contains all the required data to play the audio
 * correctly or convert the signal to another format. */
typedef struct {
    umugu_frame *frames;
    int64_t count;
    int32_t rate;     /* e.g. 48000 */
    int16_t type;     /* e.g. UMUGU_TYPE_INT16 */
    int16_t channels; /* e.g. 2 (stereo) */
    int64_t capacity; /* more than count */
} umugu_signal;

/* Node virtual functions. */
enum { UMUGU_FN_INIT, UMUGU_FN_GETSIGNAL, UMUGU_FN_RELEASE };
typedef int umugu_fn;

/* Text struct for name handling. The whole array is taken into account
   for string operations like comparison or hashing so care must be taken
   to keep them zeroed from strlen to UMUGU_NAME_LEN. */
typedef struct {
    char str[UMUGU_NAME_LEN];
} umugu_name;

/* Node base class. All the created nodes should have this type as the first
 * data member of the struct. The name is enough for obtaining its associated
 * metadata (see: umugu_node_info) and then the offsets to the interface. */
typedef struct {
    umugu_name name;
} umugu_node;

typedef int (*umugu_node_fn)(umugu_node **, umugu_signal *out);

enum umugu_var_type_e {
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
    UMUGU_TYPE_PLOTLINE, /* TODO: Delete this. And come with a better idea. */
    UMUGU_TYPE_TEXT,
    UMUGU_TYPE_BOOL,
    UMUGU_TYPE_COUNT
};

/* Node field descriptor with type metadata for external node communication
 * in a generic manner. The objective is to be able to serialize, interact
 * and draw widgets to interact with unknown nodes as long as they have
 * defined the metadata of the public variables. */
typedef struct {
    umugu_name name;
    int16_t offset_bytes; /* from struct's start */
    int16_t type;         /* enum umugu_var_type_e */
    int32_t count;        /* number of elements i.e. array */
    /* TODO: Think what to do with this two floats... only useful for
             some types, and a lot of the other types would benefit from extra
             data but I don't want to start throwing here every variable
             that could be useful when drawing widgets or whatever.
             Union maybe? */
    union {
        struct {
            float min;
            float max;
        } range;
    } misc;
} umugu_var_info;

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

/* Defines the audio fx pipeline graph. Pipelines are the scenes of umugu.
 * This is where the data that define the output sound is stored. It contains
 * the graph defining the node i/o relations between them and its data.
 * Is the struct that has to be imported/exported or generated with tools
 * like umg-editor. */
typedef struct {
    umugu_node *root;
    int64_t size_bytes;
} umugu_pipeline;

/* Audio output abstraction. The goal is to separate the audio backend
 * implementations from the library code. The implementations should be
 * able to feed the backend with the signals resulting from pipeline
 * passes using this struct and umugu_produce_signal(num_frames). */
typedef struct {
    /* The primary output of the library. Its contents (if any) are the next
     * samples to be played by the audio backend. This signal is the result
     * of the last umugu_produce_signal call. */
    umugu_signal output;

    /* TODO: Rethink the open / running decision... this smells bad and
     * I'm sure will cause bugs due to incoherent state between this
     * values and the real backend stream state. I feel like I need some
     * sort of comunication and control over the output stream but at
     * the same time it seems to me that it would be better to delete
     * the variables and forget about the stream state... if it happens
     * to fail I would have ended aborting the execution anyway. */

    /* TODO2: On second thought I think the best thing to do is to drop
     * the audio backend support and let the user implement the signal
     * playback (I will be the only user so nothing changes... maybe
     * the folder where the implementation sources will be).
     * Why am I writing this nonsense? I should go to bed. */

    /* Indicates that the stream is configured, initialized and ready to start
     * requesting and playing the obtained signal from the pipeline
     * (or already doing so).
     * In no case can the stream start playing if this variable is false. */
    int32_t open;

    /* Indicates that the stream is periodically requesting new audio fragments
     * from the pipeline as it plays them.
     * NOTE: If this variable is true, chances are that the pipeline is
     * modifying its internal state (writes) from another thread, since
     * the audio requests usually come from stream callbacks.
     * It should not be a problem if the main thread only changes parameters
     * on the nodes (typical real-time tweaking with controllers or UI)
     * because nodes only read them during signal processing, and the variables
     * where they write should be inaccessible (internal state) for the main
     * thread and widgets / inputs. The only actions that require active
     * synchronization are impot/export operations and context change, which
     * there is no point in doing while there is a stream running anyway. */
    int32_t running;

    /* Time gap (in seconds) between the stream callback call that
     * requests new signal data and the moment when the produced signal
     * will output the DAC. */
    double time_margin_sec;

    /* Opaque ptr for storing the backend stream handles */
    void *handle;
} umugu_out_stream;

typedef struct umugu_ctx {
    /* TODO: Better to have an arena and 2 types of allocators:
        - One persistent for the node params and internal state.
        - Another that resets between umugu_produce_signal calls, so
          can be used for any temporal data required during the
          pipeline's graph processing and forget about it.
          It could be a circular allocator using the arena's remaining
          space from the persistent allocs.

        None of them sould have free, so remove it too. */
    void *(*alloc)(size_t bytes);
    void (*free)(void *ptr);
    /* TODO: I think it was stupid to add this assert hook...
       I don't think it's too bad to indlude <assert.h> and use it
       in library code since it's only for debug builds.  */
    void (*assert)(int cond);
    int (*log)(const char *fmt, ...);

    /* Audio pipeline. */
    umugu_pipeline pipeline;

    /* Loaded node types metadata (non persistent). */
    umugu_node_info nodes_info[UMUGU_DEFAULT_NODE_INFO_CAPACITY];
    int32_t nodes_info_next;

    umugu_out_stream audio_output;
} umugu_ctx;

/* Internal initialization. This function should be called before any other. */
int umugu_init(void);
/* Releases resources and closes streams. */
int umugu_close(void);

/* While the stream is running, the audio callback gets called periodically
 * from another thread and the whole audio pipeline is processed in order
 * to generate the output samples required by the audio backend. */
int umugu_start_stream(void);
int umugu_stop_stream(void);

int umugu_produce_signal(int32_t number_of_samples);

/* Export the current context's pipeline to binary data.
 * Return UMUGU_SUCCESS if saved successfuly or UMUGU_ERR_FILE if file open
 * fails. */
int umugu_save_pipeline_bin(const char *filename);

/* Import the binary file as the current context's pipeline.
 * Return UMUGU_SUCCESS if loaded successfuly, UMUGU_ERR_FILE if can not open
 * the file or the file is not formatted as a pipeline binary, and
 * UMUGU_ERR_MEM if the allocation of the pipeline buffer fails. */
int umugu_load_pipeline_bin(const char *filename);

/* Search the file lib<name>.so in the rpath and load it if found.
 * Return the index of the context's node infos array where it has been copied.
 * If the dynamic object can not be found, return UMUGU_ERR_PLUG. */
int umugu_plug(const umugu_name *name);
void umugu_unplug(const umugu_name *name);

/* Node virtual dispatching.
 * @param fn Function identifier, valid definitions are prefixed with UMUGU_FN_.
 * Return UMUGU_SUCCESS if the call is performed and UMUGU_ERR otherwise. */
int umugu_node_call(int fn, umugu_node **node, umugu_signal *out);

/* Gets the node info from context's node infos.
 * Return a pointer to the node info in the context or NULL if not found. */
const umugu_node_info *umugu_node_info_find(const umugu_name *name);

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
