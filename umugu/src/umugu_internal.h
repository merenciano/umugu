#ifndef __UMUGU_INTERNAL_H__
#define __UMUGU_INTERNAL_H__

#include "umugu.h"

void umugu_init_backend(void);
void umugu_close_backend(void);

/* INTERNAL HELPERS */
static inline void um__unused(void *unused_var) { (void)unused_var; }

#endif /* __UMUGU_INTERNAL_H__ */
