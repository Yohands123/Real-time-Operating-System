/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body with inline lab1_test00a_init test
 ******************************************************************************
 */
/* USER CODE END Header */

#include "common.h"
#include "main.h"
#include "k_task.h"
#include "k_mem.h"
#include <stdio.h>

/* ------------------------------------------------------------ */
/* Local autotest replacement                                   */
/* ------------------------------------------------------------ */

static void local_pass(void)
{
    printf("PASS: lab1_test00a_init\r\n");

    while (1)
    {
    }
}

static void local_fail(const char *msg)
{
    printf("FAIL: %s\r\n", msg);

    while (1)
    {
    }
}

static void local_assert(int condition, const char *msg)
{
    if (!condition)
    {
        local_fail(msg);
    }
}

static void local_assert_eq_u32(U32 actual, U32 expected, const char *msg)
{
    if (actual != expected)
    {
        printf("Expected 0x%x, got 0x%x\r\n", expected, actual);
        local_fail(msg);
    }
}

/* ------------------------------------------------------------ */
/* Inline lab1_test00a_init contents                            */
/* ------------------------------------------------------------ */

int task1_ran = 0;

void task1_main(void *args)
{
    task_t task = osGetTID();
    TCB tcb;

    if (osTaskInfo(task, &tcb) != RTX_OK)
    {
        local_fail("task1 osTaskInfo failed");
    }

    local_assert_eq_u32((U32)tcb.tid, (U32)task, "task1 TID mismatch");
    local_assert_eq_u32((U32)tcb.ptask, (U32)task1_main, "task1 ptask mismatch");
    local_assert(tcb.stack_high != 0, "task1 stack_high missing");
    local_assert_eq_u32((U32)tcb.state, (U32)RUNNING, "task1 state not RUNNING");

    task1_ran = 1;

    osYield();
    osYield();

    osTaskExit();

    local_fail("task1 osTaskExit returned");
}

void task2_main(void *args)
{
    task_t task = osGetTID();
    TCB tcb;

    if (osTaskInfo(task, &tcb) != RTX_OK)
    {
        local_fail("task2 osTaskInfo failed");
    }

    local_assert_eq_u32((U32)tcb.tid, (U32)task, "task2 TID mismatch");
    local_assert_eq_u32((U32)tcb.ptask, (U32)task2_main, "task2 ptask mismatch");
    local_assert(tcb.stack_high != 0, "task2 stack_high missing");
    local_assert_eq_u32((U32)tcb.state, (U32)RUNNING, "task2 state not RUNNING");
    local_assert(task1_ran != 0, "task2 ran before task1");

    local_pass();

    osYield();
    osYield();

    osTaskExit();

    local_fail("task2 osTaskExit returned");
}

void test_main(void)
{
    printf("---- lab1_test00a_init local ----\r\n");

    osKernelInit();

    TCB tcb;
    tcb.ptask = task1_main;
    tcb.stack_high = 0;
    tcb.stack_size = 0x200;
    tcb.state = READY;
    tcb.tid = 0xff;

    int err = osCreateTask(&tcb);

    if (err != RTX_OK)
    {
        local_fail("create task 1 failed");
    }

    if (tcb.tid == 0xff)
    {
        local_fail("task 1 no tid assigned");
    }

    TCB tcb2;
    tcb2.ptask = task2_main;
    tcb2.stack_high = 0;
    tcb2.stack_size = 0x200;
    tcb2.state = READY;
    tcb2.tid = 0xff;

    err = osCreateTask(&tcb2);

    if (err != RTX_OK)
    {
        local_fail("create task 2 failed");
    }

    if (tcb2.tid == 0xff)
    {
        local_fail("task 2 no tid assigned");
    }

    printf("created task1 tid=%u stack_high=0x%x stack_size=0x%x\r\n",
           tcb.tid,
           tcb.stack_high,
           tcb.stack_size);

    printf("created task2 tid=%u stack_high=0x%x stack_size=0x%x\r\n",
           tcb2.tid,
           tcb2.stack_high,
           tcb2.stack_size);

    osKernelStart();

    local_fail("kernel start returned");
}

/* ------------------------------------------------------------ */
/* main                                                         */
/* ------------------------------------------------------------ */

int main(void)
{
    /* MCU Configuration: Don't change this or the whole chip won't work! */

    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART2_UART_Init();

    test_main();

    while (1)
    {
    }
}
