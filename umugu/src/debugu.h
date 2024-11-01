#ifndef __UMUGU_DEBUG_H__
#define __UMUGU_DEBUG_H__

#include "umugu/umugu.h"

int umugu_node_print(umugu_node *node);
int umugu_pipeline_print(void);
int umugu_mem_arena_print(void);
int umugu_in_signal_print(void);
int umugu_out_signal_print(void);

#endif /* __UMUGU_DEBUG_H__ */
