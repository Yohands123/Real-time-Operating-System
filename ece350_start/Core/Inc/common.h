#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#include <stdint.h>

#define TID_NULL            0U
#define MAX_TASKS           16U
#define STACK_SIZE          0x200U
#define DEFAULT_DEADLINE    5

#define DORMANT             0U
#define READY               1U
#define RUNNING             2U
#define SLEEPING            3U

#define RTX_OK              0
#define RTX_ERR            -1

typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t  U8;
typedef uint32_t task_t;

typedef struct task_control_block {
    void (*ptask)(void *args); /* entry address */
    U32 stack_high;            /* high address of the task stack */
    task_t tid;                /* task ID */
    U8 state;                  /* DORMANT, READY, RUNNING, or SLEEPING */
    U32 stack_size;            /* bytes, rounded to a multiple of 8 */
    U32 stack_ptr;             /* saved PSP; keep at byte offset 20 */

    /* Deliverable 3 fields. */
    int deadline;              /* task period/deadline in milliseconds */
    int remaining_time;        /* milliseconds until the current deadline */
    int sleep_time;            /* milliseconds until a sleeping task wakes */
} TCB;

#endif /* INC_COMMON_H_ */