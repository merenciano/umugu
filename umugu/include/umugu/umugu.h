/*
    Venga, a ver si te buscas una musiquilla guapa, no colega?

    TODO(Err)
    TODO(qol)
    TODO:
        - Plug/unplug dyamic libs with external nodes.
        - Adapt to any wav config (mono, stereo, samples per sec, data size..)
        - Wav file player split into file reader and decoder
        - Think about data separation: the audio stream buffer writes to state
            temp data and reads from config data, and the main thread only
            writes to config data (from the editor only) and the only data
            that matters for import/export is the config data.

        - Organize all the config and decide how it's done. File? Defines? Context?

    Symbol prefix:
        - Public interface: umugu_
        - Private impl:     um__
*/

#ifndef __UMUGU_H__
#define __UMUGU_H__

#include <stddef.h>
#include <stdint.h>

#define UMUGU_VERSION_MAJOR 0
#define UMUGU_VERSION_MINOR 1
#define UMUGU_VERSION_PATCH 0

#define UMUGU_SAMPLE_RATE 48000
#define UMUGU_NAME_LEN 32
#define UMUGU_PATH_LEN 64
#define UMUGU_ERR_MSG_LEN 2048

#ifdef __cplusplus
extern "C" {
#endif

enum {
    /* Negative codes (commonly used in positive-only variables) */
    UMUGU_INVALID = -2,
    UMUGU_DEFAULT,

    /* Definitions */
    UMUGU_NONE = 0,
    UMUGU_SUCCESS,

    UMUGU_LOG = (1 << 8),
    UMUGU_LOG_VERBOSE,
    UMUGU_LOG_INFO,
    UMUGU_LOG_WARN,
    UMUGU_LOG_ERR,

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
    UMUGU_ERR_PORTAUDIO,
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

typedef void (*umugu_node_fn)(umugu_node **, umugu_signal *out);

typedef struct {
    umugu_node *root;
    int64_t size_bytes;
} umugu_pipeline;

typedef struct umugu_ctx {
    void *(*alloc)(size_t bytes);
    void (*free)(void *ptr);
    void (*assert)(int cond);
    void (*log)(int lvl, const char *fmt, ...);
    void (*abort)(void);

    /* Audio pipeline. */
    umugu_pipeline pipeline;

    const int32_t sample_rate;
    const int32_t audio_src_sample_capacity;
    int32_t err;
    char err_msg[UMUGU_ERR_MSG_LEN];
} umugu_ctx;

void umugu_init(void);
void umugu_close(void);

/* While the stream is running, the audio callback gets called periodically
 * from another thread and the whole audio pipeline is processed in order
 * to generate the output samples required by the audio backend. */
void umugu_start_stream(void);
void umugu_stop_stream(void);

/* Import/export to binary data. The pipeline instance used in both cases
 * is the one from the current context. */
void umugu_save_pipeline_bin(const char *filename);
void umugu_load_pipeline_bin(const char *filename);

/* Dynamic link of external shared libs containing audio nodes. */
int umugu_plug(umugu_name *name);
void umugu_unplug(umugu_name *name);
/* Node virtual dispatching. */
void umugu_node_call(umugu_code fn, umugu_node **node, umugu_signal *out);

/* Active context instance manipulation. */
void umugu_set_context(umugu_ctx *new_ctx);
umugu_ctx *umugu_get_context(void);

#ifdef __cplusplus
}
#endif

#endif /* __UMUGU_H__ */
