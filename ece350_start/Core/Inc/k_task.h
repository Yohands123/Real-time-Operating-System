#ifndef INC_K_TASK_H_
#define INC_K_TASK_H_

#include <stddef.h>
#include "common.h"

extern TCB tcbs[MAX_TASKS];
extern task_t current_task;
extern TCB *current_tcb;
extern U32 active_tasks;

void osKernelInit(void);
int osKernelStart(void);

int osCreateTask(TCB *task);
int osCreateDeadlineTask(int deadline, TCB *task);

void osYield(void);
void osSleep(int timeInMs);
void osPeriodYield(void);

int osSetDeadline(int deadline, task_t TID);
int osTaskExit(void);
int osTaskInfo(task_t TID, TCB *task_copy);
task_t osGetTID(void);

/* Deliverable 2 memory API SVC wrappers. */
int osMemInit(void);
void *osMemAlloc(size_t size);
int osMemDealloc(void *ptr);
int osMemCountExtFrag(size_t size);

/* Called by assembly and SysTick. */
void SVC_Handler_Main(U32 *svc_args);
void k_scheduler(void);
void k_tick(void);

/* Used by memory manager. */
int k_kernel_is_initialized(void);

#endif /* INC_K_TASK_H_ */