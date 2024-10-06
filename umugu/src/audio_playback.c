#include "umugu_internal.h"

#ifdef UMUGU_BACKEND_PORTAUDIO
#ifdef UMUGU_BACKEND_STDOUT
#error "Define only one audio backend."
#endif

#include "backends/portaudio_impl.h"

#else
/* It is possible that this is not defined since this is the fallback.
 * It does not depend on anything more than stdio and it is still useful
 * if combined e.g. with aplay, it is really convenient to pipe umugu
 * through aplay in the terminal and listen. */ 
#define UMUGU_BACKEND_STDOUT
#include "backends/stdout_impl.h"

#endif
