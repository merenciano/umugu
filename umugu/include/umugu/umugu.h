/*
    Venga, a ver si te buscas una musiquilla guapa, no colega?

    TODO(Err)
    TODO(qol)
    TODO:
        - Better signals (channels and sample rate)
        - Sanitize build config.
        - Profiling build config.
        - Doxygen.
        - Adapt to any wav config (mono, stereo, samples per sec, data size..)
        - Wav file player split into file reader and decoder
        - Think about data separation: the audio stream buffer writes to state
            temp data and reads from config data, and the main thread only
            writes to config data (from the editor only) and the only data
            that matters for import/export is the config data.

        - Organize all the config and decide how it's done. File? Defines?
        - Remove error msg.. use logs instead.
        - Remove error code from ctx, use return values.
        - Change pointers with arena offsets in serialized structs (I want .bin
   compat between 32-64 bit systems).

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
#define UMUGU_NAME_LEN 32
#define UMUGU_PATH_LEN 64

#ifdef __cplusplus
extern "C" {
#endif

enum {
    UMUGU_NONE = 0,
    UMUGU_SUCCESS,

    UMUGU_FN = (1 << 9),
    UMUGU_FN_INIT,
    UMUGU_FN_GETSIGNAL,
    UMUGU_FN_RELEASE,

    /* Error codes */
    UMUGU_ERR_LAST = -32768,
    UMUGU_ERR_ARGS,
    UMUGU_ERR_MEM,
    UMUGU_ERR_FILE,
    UMUGU_ERR_PLUG,
    UMUGU_ERR,
};

typedef int32_t umugu_code; /* Values from umugu_codes are expected. */

typedef struct {
    char str[UMUGU_NAME_LEN];
} umugu_name;

typedef struct {
    float left;
    float right;
} umugu_sample;

typedef struct {
    umugu_sample *samples;
    int64_t count;
} umugu_signal;

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
    /* ... */
    UMUGU_TYPE_BOOL,
    UMUGU_TYPE_INT8 = 64,
    UMUGU_TYPE_INT16,
    UMUGU_TYPE_INT32,
    UMUGU_TYPE_INT64,
    UMUGU_TYPE_FLOAT,
    UMUGU_TYPE_PLOTLINE,
    UMUGU_TYPE_TEXT,
    UMUGU_TYPE_SIGNAL, /* Audio signal i.e. umugu_signal */
    UMUGU_TYPE_COUNT
};

typedef struct {
    umugu_name name;
    int16_t offset_bytes; /* from struct's start */
    int16_t type;         /* enum umugu_var_type_e */
    int32_t count;
    float range_min;
    float range_max;
} umugu_var_info;

typedef struct {
    umugu_name name;
    int32_t size_bytes;
    int32_t var_count;
    umugu_node_fn (*getfn)(umugu_code fn);
    const umugu_var_info *vars; /* field metadata */
    void *plug_handle;
} umugu_node_info;

typedef struct {
    umugu_node *root;
    int64_t size_bytes;
} umugu_pipeline;

typedef struct umugu_ctx {
    void *(*alloc)(size_t bytes);
    void (*free)(void *ptr);
    void (*assert)(int cond);
    int (*log)(const char *fmt, ...);
    void (*abort)(void); /* TODO: Remove? */

    /* Audio pipeline. */
    umugu_pipeline pipeline;

    /* Loaded node types metadata. */
    umugu_node_info *nodes_info;
    int32_t nodes_info_next;
    int32_t nodes_info_capacity;

    const int32_t sample_rate;
    const int32_t audio_src_sample_capacity;
} umugu_ctx;

/* Internal initialization. This function should be called before any other. */
void umugu_init(void);
/* Releases resources and closes streams. */
void umugu_close(void);

/* While the stream is running, the audio callback gets called periodically
 * from another thread and the whole audio pipeline is processed in order
 * to generate the output samples required by the audio backend. */
void umugu_start_stream(void);
void umugu_stop_stream(void);

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
 * Return UMUGU_SUCCESS if the call is performed and UMUGU_ERR otherwise. */
int umugu_node_call(umugu_code fn, umugu_node **node, umugu_signal *out);

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
