#ifndef INC_K_MEM_H_
#define INC_K_MEM_H_

#include <stddef.h>
#include "common.h"

int k_mem_init(void);
void *k_mem_alloc(size_t size);
int k_mem_dealloc(void *ptr);
int k_mem_count_extfrag(size_t size);

/* Internal helpers used for dynamically allocated task stacks. */
int k_mem_is_initialized(void);
void *k_mem_alloc_owner(size_t size, task_t owner);
int k_mem_release_task(task_t owner);

#endif /* INC_K_MEM_H_ */