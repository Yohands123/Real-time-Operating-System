#include "common.h"
#include "k_task.h"
#include "k_mem.h"

#include <stddef.h>
#include <stdint.h>

#define SCB_ICSR_ADDRESS      0xE000ED04U
#define SCB_SHPR3_ADDRESS     0xE000ED20U
#define SYSTICK_VAL_ADDRESS   0xE000E018U
#define PENDSVSET_BIT         (1UL << 28)

#define SCB_ICSR              (*(volatile U32 *)SCB_ICSR_ADDRESS)
#define SCB_SHPR3             (*(volatile U32 *)SCB_SHPR3_ADDRESS)
#define SYSTICK_VAL           (*(volatile U32 *)SYSTICK_VAL_ADDRESS)

TCB tcbs[MAX_TASKS];
task_t current_task = TID_NULL;
TCB *current_tcb = NULL;
U32 active_tasks = 0U;

static int kernel_initialized = 0;
static volatile int kernel_running = 0;

/*
 * The NULL task uses a static stack because it always exists and belongs
 * permanently to the kernel.
 */
static U8 null_stack[STACK_SIZE] __attribute__((aligned(8)));

_Static_assert(
    offsetof(TCB, state) == 12U,
    "os_cpu.s expects TCB.state at byte offset 12"
);

_Static_assert(
    offsetof(TCB, stack_ptr) == 20U,
    "os_cpu.s expects TCB.stack_ptr at byte offset 20"
);



static void request_context_switch(void)
{
    /*
     * Set the PendSV pending bit.
     *
     * Writing 1 to bit 28 requests PendSV without disturbing the other
     * interrupt-control bits.
     */
    SCB_ICSR = PENDSVSET_BIT;
}

static int valid_user_task(task_t tid)
{
    return tid > TID_NULL && tid < MAX_TASKS;
}

/*
 * Return nonzero when candidate has a higher EDF priority than incumbent.
 *
 * Smaller remaining deadline has higher priority. If the remaining deadlines
 * are equal, the task with the smaller TID has higher priority.
 */
static int task_outranks(task_t candidate, task_t incumbent)
{
    int candidate_time;
    int incumbent_time;

    if (candidate == TID_NULL) {
        return 0;
    }

    if (incumbent == TID_NULL) {
        return 1;
    }

    candidate_time = tcbs[candidate].remaining_time;
    incumbent_time = tcbs[incumbent].remaining_time;

    if (candidate_time < 0) {
        candidate_time = 0;
    }

    if (incumbent_time < 0) {
        incumbent_time = 0;
    }

    if (candidate_time != incumbent_time) {
        return candidate_time < incumbent_time;
    }

    return candidate < incumbent;
}

static void null_task(void *args)
{
    (void)args;

    while (1) {
        /*
         * Wait for an interrupt. SysTick will wake the processor whenever
         * sleeping tasks need their timing information updated.
         */
        __asm volatile("wfi");
    }
}

/*
 * Build an initial exception/context frame.
 *
 * PendSV restores R4-R11 manually. The processor restores R0-R3, R12, LR, PC,
 * and xPSR automatically during exception return.
 */
static U32 *init_task_stack(void (*task_func)(void *), U32 stack_high)
{
    U32 *stack_pointer = (U32 *)(uintptr_t)stack_high;
    int index;

    /* Hardware exception frame. */
    *(--stack_pointer) = (1UL << 24);                  /* xPSR */
    *(--stack_pointer) = (U32)(uintptr_t)task_func;    /* PC */
    *(--stack_pointer) = (U32)(uintptr_t)osTaskExit;   /* LR */
    *(--stack_pointer) = 0U;                          /* R12 */
    *(--stack_pointer) = 0U;                          /* R3 */
    *(--stack_pointer) = 0U;                          /* R2 */
    *(--stack_pointer) = 0U;                          /* R1 */
    *(--stack_pointer) = 0U;                          /* R0 */

    /* Software-saved frame for R4-R11. */
    for (index = 0; index < 8; ++index) {
        *(--stack_pointer) = 0U;
    }

    return stack_pointer;
}

static void initialize_tcb(TCB *tcb, task_t tid)
{
    tcb->ptask = NULL;
    tcb->stack_high = 0U;
    tcb->tid = tid;
    tcb->state = DORMANT;
    tcb->stack_size = 0U;
    tcb->stack_ptr = 0U;
    tcb->deadline = 0;
    tcb->remaining_time = 0;
    tcb->sleep_time = 0;
}

static int ensure_memory_manager(void)
{
    if (k_mem_is_initialized()) {
        return RTX_OK;
    }

    return k_mem_init();
}



static int k_create_deadline_task(int deadline, TCB *task)
{
    task_t tid;
    U32 stack_size;
    void *stack_base;
    U32 stack_high;

    if (!kernel_initialized ||
        task == NULL ||
        task->ptask == NULL ||
        deadline <= 0 ||
        task->stack_size < STACK_SIZE) {
        return RTX_ERR;
    }

    /*
     * Find the lowest available user TID. TID 0 is permanently reserved for
     * the NULL task.
     */
    tid = MAX_TASKS;

    for (task_t index = 1U; index < MAX_TASKS; ++index) {
        if (tcbs[index].state == DORMANT) {
            tid = index;
            break;
        }
    }

    if (tid == MAX_TASKS) {
        return RTX_ERR;
    }

    /*
     * Round the requested stack size upward to a multiple of eight.
     */
    if (task->stack_size > UINT32_MAX - 7U) {
        return RTX_ERR;
    }

    stack_size = (task->stack_size + 7U) & ~7U;

    if (ensure_memory_manager() != RTX_OK) {
        return RTX_ERR;
    }

    /*
     * Allocate the stack directly to the new task rather than to the task
     * that called osCreateDeadlineTask.
     */
    stack_base = k_mem_alloc_owner(stack_size, tid);

    if (stack_base == NULL) {
        return RTX_ERR;
    }

    stack_high = (U32)((uintptr_t)stack_base + stack_size);

    tcbs[tid].ptask = task->ptask;
    tcbs[tid].stack_high = stack_high;
    tcbs[tid].tid = tid;
    tcbs[tid].state = READY;
    tcbs[tid].stack_size = stack_size;

    tcbs[tid].stack_ptr =
        (U32)(uintptr_t)init_task_stack(task->ptask, stack_high);

    tcbs[tid].deadline = deadline;
    tcbs[tid].remaining_time = deadline;
    tcbs[tid].sleep_time = 0;

    /*
     * Return every assigned TCB field to the caller.
     */
    *task = tcbs[tid];

    ++active_tasks;

    /*
     * A task created after the kernel starts may have an earlier deadline
     * than the currently executing task.
     */
    if (kernel_running &&
        current_tcb != NULL &&
        task_outranks(tid, current_task)) {
        request_context_switch();
    }

    return RTX_OK;
}

static int k_task_info(task_t tid, TCB *task_copy)
{
    if (task_copy == NULL ||
        tid >= MAX_TASKS ||
        tcbs[tid].state == DORMANT) {
        return RTX_ERR;
    }

    *task_copy = tcbs[tid];

    return RTX_OK;
}

static int k_task_exit(void)
{
    if (!kernel_running ||
        !valid_user_task(current_task) ||
        current_tcb == NULL ||
        current_tcb->state != RUNNING) {
        return RTX_ERR;
    }

    /*
     * Do not free the stack here because the CPU is still executing on that
     * stack. The scheduler frees it after PendSV saves the final context.
     */
    current_tcb->state = DORMANT;

    if (active_tasks > 1U) {
        --active_tasks;
    }

    request_context_switch();

    return RTX_OK;
}

static task_t k_get_tid(void)
{
    if (!kernel_running || current_tcb == NULL) {
        return TID_NULL;
    }

    return current_task;
}

static int k_kernel_start(void)
{
    if (!kernel_initialized || kernel_running) {
        return RTX_ERR;
    }

    /*
     * Reset all task timers immediately before scheduling begins.
     */
    for (task_t tid = 1U; tid < MAX_TASKS; ++tid) {
        if (tcbs[tid].state == READY) {
            tcbs[tid].remaining_time = tcbs[tid].deadline;
        }
    }

    kernel_running = 1;

    /*
     * Reset the SysTick current-value register so timing starts from a clean
     * counter value.
     */
    SYSTICK_VAL = 0U;

    request_context_switch();

    return RTX_OK;
}

static void k_yield(void)
{
    if (!kernel_running || current_tcb == NULL) {
        return;
    }

    /*
     * Cooperative yielding starts a fresh deadline/timeslice for the calling
     * task.
     */
    if (valid_user_task(current_task) &&
        current_tcb->state == RUNNING) {
        current_tcb->remaining_time = current_tcb->deadline;
    }

    request_context_switch();
}

static void k_sleep(int time_in_ms)
{
    if (!kernel_running ||
        !valid_user_task(current_task) ||
        current_tcb == NULL ||
        current_tcb->state != RUNNING) {
        return;
    }

    if (time_in_ms <= 0) {
        /*
         * Treat a nonpositive sleep time as a regular yield.
         */
        current_tcb->remaining_time = current_tcb->deadline;
    } else {
        current_tcb->sleep_time = time_in_ms;
        current_tcb->state = SLEEPING;
    }

    request_context_switch();
}

static void k_period_yield(void)
{
    int sleep_time;

    if (!kernel_running ||
        !valid_user_task(current_task) ||
        current_tcb == NULL ||
        current_tcb->state != RUNNING) {
        return;
    }

    /*
     * The task sleeps for the remainder of its current period.
     */
    sleep_time = current_tcb->remaining_time;

    /*
     * If the task has already reached or passed its deadline, wait for one
     * complete new period.
     */
    if (sleep_time <= 0) {
        sleep_time = current_tcb->deadline;
    }

    current_tcb->sleep_time = sleep_time;
    current_tcb->state = SLEEPING;

    request_context_switch();
}

static int k_set_deadline(int deadline, task_t tid)
{
    /*
     * The manual specifies that the target task must be READY. A task cannot
     * use this operation to change its own currently RUNNING deadline.
     */
    if (!kernel_initialized ||
        deadline <= 0 ||
        !valid_user_task(tid) ||
        tcbs[tid].state != READY) {
        return RTX_ERR;
    }

    if (kernel_running && tid == current_task) {
        return RTX_ERR;
    }

    tcbs[tid].deadline = deadline;
    tcbs[tid].remaining_time = deadline;

    /*
     * If the updated task now outranks the current task, request immediate
     * pre-emption.
     */
    if (kernel_running &&
        current_tcb != NULL &&
        task_outranks(tid, current_task)) {
        request_context_switch();
    }

    return RTX_OK;
}



int k_kernel_is_initialized(void)
{
    return kernel_initialized;
}

void osKernelInit(void)
{
    U32 null_stack_high;

    if (kernel_initialized) {
        return;
    }

    for (task_t tid = 0U; tid < MAX_TASKS; ++tid) {
        initialize_tcb(&tcbs[tid], tid);
    }

    null_stack_high =
        (U32)((uintptr_t)null_stack + (uintptr_t)sizeof(null_stack));

    tcbs[TID_NULL].ptask = null_task;
    tcbs[TID_NULL].stack_high = null_stack_high;
    tcbs[TID_NULL].tid = TID_NULL;
    tcbs[TID_NULL].state = READY;
    tcbs[TID_NULL].stack_size = STACK_SIZE;

    tcbs[TID_NULL].stack_ptr =
        (U32)(uintptr_t)init_task_stack(null_task, null_stack_high);

    /*
     * The NULL task always has the lowest possible EDF priority.
     */
    tcbs[TID_NULL].deadline = INT32_MAX;
    tcbs[TID_NULL].remaining_time = INT32_MAX;
    tcbs[TID_NULL].sleep_time = 0;

    current_task = TID_NULL;
    current_tcb = NULL;
    active_tasks = 1U;
    kernel_running = 0;

    /*
     * Bits 23:16 control PendSV priority.
     * Bits 31:24 control SysTick priority.
     *
     * Both are set to the lowest priority. Because they have the same
     * priority, SysTick cannot interrupt PendSV while the context switch is
     * being performed.
     *
     * SVC keeps its higher default priority.
     */
    SCB_SHPR3 =
        (SCB_SHPR3 & 0x0000FFFFU) |
        (0xFFU << 16) |
        (0xFFU << 24);

    kernel_initialized = 1;
}

/* -------------------------------------------------------------------------- */
/* EDF scheduler                                                              */
/* -------------------------------------------------------------------------- */

void k_scheduler(void)
{
    task_t previous = current_task;
    task_t selected = TID_NULL;

    if (current_tcb != NULL) {
        if (current_tcb->state == RUNNING) {
            /*
             * A running task becomes READY whenever PendSV is entered because
             * of yielding, pre-emption, or deadline expiration.
             */
            current_tcb->state = READY;
        } else if (current_tcb->state == DORMANT &&
                   valid_user_task(previous)) {
            /*
             * PendSV has already saved and switched away from this task's PSP,
             * so it is safe to free its stack and all other blocks owned by
             * the terminated task.
             */
            if (k_mem_is_initialized()) {
                (void)k_mem_release_task(previous);
            }

            tcbs[previous].ptask = NULL;
            tcbs[previous].stack_high = 0U;
            tcbs[previous].stack_size = 0U;
            tcbs[previous].stack_ptr = 0U;
            tcbs[previous].deadline = 0;
            tcbs[previous].remaining_time = 0;
            tcbs[previous].sleep_time = 0;
        }
    }

    /*
     * Earliest Deadline First:
     *
     * 1. Select the READY user task with the smallest remaining deadline.
     * 2. Break equal-deadline ties using the lowest TID.
     * 3. Select the NULL task when no user task is READY.
     */
    for (task_t tid = 1U; tid < MAX_TASKS; ++tid) {
        if (tcbs[tid].state == READY &&
            (selected == TID_NULL ||
             task_outranks(tid, selected))) {
            selected = tid;
        }
    }

    if (selected == TID_NULL) {
        selected = TID_NULL;
    } else if (tcbs[selected].remaining_time <= 0) {
        /*
         * If a READY task reached zero while waiting, give it a complete new
         * deadline period when it is selected.
         */
        tcbs[selected].remaining_time = tcbs[selected].deadline;
    }

    current_task = selected;
    current_tcb = &tcbs[selected];
    current_tcb->state = RUNNING;
}



void k_tick(void)
{
    int should_switch = 0;

    if (!kernel_running || current_tcb == NULL) {
        return;
    }

    /*
     * One call represents one millisecond because STM32Cube configures
     * SysTick at a 1 ms period.
     */
    for (task_t tid = 1U; tid < MAX_TASKS; ++tid) {
        if (tcbs[tid].state == SLEEPING) {
            if (tcbs[tid].sleep_time > 0) {
                --tcbs[tid].sleep_time;
            }

            if (tcbs[tid].sleep_time <= 0) {
                tcbs[tid].sleep_time = 0;
                tcbs[tid].remaining_time = tcbs[tid].deadline;
                tcbs[tid].state = READY;
                should_switch = 1;
            }
        } else if (tcbs[tid].state == READY ||
                   tcbs[tid].state == RUNNING) {
            /*
             * Deadlines approach for every READY and RUNNING task, not only
             * the task currently using the CPU.
             */
            if (tcbs[tid].remaining_time > 0) {
                --tcbs[tid].remaining_time;
            }
        }
    }

    /*
     * When the running task exhausts its current deadline/timeslice, start its
     * next period and invoke EDF again.
     */
    if (valid_user_task(current_task) &&
        current_tcb->state == RUNNING &&
        current_tcb->remaining_time <= 0) {
        current_tcb->remaining_time = current_tcb->deadline;
        should_switch = 1;
    }

    /*
     * If the NULL task is running and any user task becomes READY, the user
     * task must immediately replace the NULL task.
     */
    if (current_task == TID_NULL) {
        for (task_t tid = 1U; tid < MAX_TASKS; ++tid) {
            if (tcbs[tid].state == READY) {
                should_switch = 1;
                break;
            }
        }
    } else {
        /*
         * A READY task may now have an earlier remaining deadline than the
         * running task.
         */
        for (task_t tid = 1U; tid < MAX_TASKS; ++tid) {
            if (tcbs[tid].state == READY &&
                task_outranks(tid, current_task)) {
                should_switch = 1;
                break;
            }
        }
    }

    if (should_switch) {
        request_context_switch();
    }
}


void osYield(void)
{
    __asm volatile(
        "svc #0"
        :
        :
        : "memory"
    );
}

int osCreateTask(TCB *task)
{
    register U32 r0 __asm("r0") = (U32)(uintptr_t)task;

    __asm volatile(
        "svc #1"
        : "+r"(r0)
        :
        : "memory"
    );

    return (int)r0;
}

int osTaskInfo(task_t tid, TCB *task_copy)
{
    register U32 r0 __asm("r0") = (U32)tid;
    register U32 r1 __asm("r1") = (U32)(uintptr_t)task_copy;

    __asm volatile(
        "svc #2"
        : "+r"(r0)
        : "r"(r1)
        : "memory"
    );

    return (int)r0;
}

int osTaskExit(void)
{
    register U32 r0 __asm("r0");

    __asm volatile(
        "svc #3"
        : "=r"(r0)
        :
        : "memory"
    );

    return (int)r0;
}

task_t osGetTID(void)
{
    register U32 r0 __asm("r0");

    __asm volatile(
        "svc #4"
        : "=r"(r0)
        :
        : "memory"
    );

    return (task_t)r0;
}

int osMemInit(void)
{
    register U32 r0 __asm("r0");

    __asm volatile(
        "svc #5"
        : "=r"(r0)
        :
        : "memory"
    );

    return (int)r0;
}

void *osMemAlloc(size_t size)
{
    register U32 r0 __asm("r0") = (U32)size;

    __asm volatile(
        "svc #6"
        : "+r"(r0)
        :
        : "memory"
    );

    return (void *)(uintptr_t)r0;
}

int osMemDealloc(void *ptr)
{
    register U32 r0 __asm("r0") = (U32)(uintptr_t)ptr;

    __asm volatile(
        "svc #7"
        : "+r"(r0)
        :
        : "memory"
    );

    return (int)r0;
}

int osMemCountExtFrag(size_t size)
{
    register U32 r0 __asm("r0") = (U32)size;

    __asm volatile(
        "svc #8"
        : "+r"(r0)
        :
        : "memory"
    );

    return (int)r0;
}

int osKernelStart(void)
{
    register U32 r0 __asm("r0");

    __asm volatile(
        "svc #9"
        : "=r"(r0)
        :
        : "memory"
    );

    return (int)r0;
}

void osSleep(int timeInMs)
{
    register U32 r0 __asm("r0") = (U32)timeInMs;

    __asm volatile(
        "svc #10"
        : "+r"(r0)
        :
        : "memory"
    );
}

void osPeriodYield(void)
{
    __asm volatile(
        "svc #11"
        :
        :
        : "memory"
    );
}

int osSetDeadline(int deadline, task_t tid)
{
    register U32 r0 __asm("r0") = (U32)deadline;
    register U32 r1 __asm("r1") = (U32)tid;

    __asm volatile(
        "svc #12"
        : "+r"(r0)
        : "r"(r1)
        : "memory"
    );

    return (int)r0;
}

int osCreateDeadlineTask(int deadline, TCB *task)
{
    register U32 r0 __asm("r0") = (U32)deadline;
    register U32 r1 __asm("r1") = (U32)(uintptr_t)task;

    __asm volatile(
        "svc #13"
        : "+r"(r0)
        : "r"(r1)
        : "memory"
    );

    return (int)r0;
}



void SVC_Handler_Main(U32 *svc_args)
{
    /*
     * The SVC number is the immediate byte stored two bytes before the saved
     * return address.
     */
    U8 svc_number = ((U8 *)(uintptr_t)svc_args[6])[-2];

    switch (svc_number) {
    case 0:
        k_yield();
        break;

    case 1:
        svc_args[0] = (U32)k_create_deadline_task(
            DEFAULT_DEADLINE,
            (TCB *)(uintptr_t)svc_args[0]
        );
        break;

    case 2:
        svc_args[0] = (U32)k_task_info(
            (task_t)svc_args[0],
            (TCB *)(uintptr_t)svc_args[1]
        );
        break;

    case 3:
        svc_args[0] = (U32)k_task_exit();
        break;

    case 4:
        svc_args[0] = (U32)k_get_tid();
        break;

    case 5:
        svc_args[0] = (U32)k_mem_init();
        break;

    case 6:
        svc_args[0] =
            (U32)(uintptr_t)k_mem_alloc((size_t)svc_args[0]);
        break;

    case 7:
        svc_args[0] =
            (U32)k_mem_dealloc((void *)(uintptr_t)svc_args[0]);
        break;

    case 8:
        svc_args[0] =
            (U32)k_mem_count_extfrag((size_t)svc_args[0]);
        break;

    case 9:
        svc_args[0] = (U32)k_kernel_start();
        break;

    case 10:
        k_sleep((int)svc_args[0]);
        break;

    case 11:
        k_period_yield();
        break;

    case 12:
        svc_args[0] = (U32)k_set_deadline(
            (int)svc_args[0],
            (task_t)svc_args[1]
        );
        break;

    case 13:
        svc_args[0] = (U32)k_create_deadline_task(
            (int)svc_args[0],
            (TCB *)(uintptr_t)svc_args[1]
        );
        break;

    default:
        break;
    }
}