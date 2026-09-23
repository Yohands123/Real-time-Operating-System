# Real-Time Operating System (RTX)

A **bare-metal real-time operating system** for the **ARM Cortex-M4**, developed for the STM32F401RE / STM32F411RE platform.

This project builds an RTX kernel from the ground up, progressing from **cooperative multitasking** to **dynamic memory management** and finally to **pre-emptive real-time scheduling**.

## Overview

The kernel provides a small multiprogramming environment with a clear separation between **kernel space** and **user tasks**. Core operating-system services are exposed through **Supervisor Calls (SVC)**, while task context switching is performed with the Cortex-M **PendSV** exception.

The project is organized around three major subsystems:

- **Task Management** — create, schedule, yield, inspect, and terminate tasks
- **Memory Management** — dynamically allocate and free heap memory using a custom allocator
- **Real-Time Scheduling** — pre-empt tasks using **SysTick** and schedule them with **Earliest Deadline First (EDF)**

## Key Features

### Multitasking

- Supports up to a fixed number of concurrent tasks
- **Task Control Blocks (TCBs)** maintain task metadata and execution state
- Task states include **DORMANT**, **READY**, **RUNNING**, and later **SLEEPING**
- Cooperative **round-robin scheduling**
- Task creation and termination at runtime
- Explicit yielding through `osYield()`
- Context switching using **PendSV**
- Kernel operations executed through **SVC calls**
- Per-task stack management
- Task information lookup and Task ID tracking

Core APIs include:

```c
void osKernelInit(void);
int osCreateTask(TCB *task);
int osKernelStart(void);
void osYield(void);
int osTaskInfo(task_t TID, TCB *task_copy);
task_t osGetTID(void);
int osTaskExit(void);
```

### Dynamic Memory Management

The RTX includes a custom heap allocator rather than relying on the C standard library.

Features include:

- **First Fit** allocation
- Free-list based heap management
- **4-byte memory alignment**
- Per-block metadata
- Memory ownership tracking by task
- Immediate **coalescing** of adjacent free blocks
- Detection of invalid or repeated frees
- External fragmentation analysis
- Dynamic allocation without `malloc()`

Memory-management APIs include:

```c
int k_mem_init(void);
void *k_mem_alloc(size_t size);
int k_mem_dealloc(void *ptr);
int k_mem_count_extfrag(size_t size);
```

### Pre-emptive Real-Time Scheduling

The final stage extends the kernel into a real-time scheduler.

- **SysTick** provides a 1 ms timing source
- Pre-emptive **Earliest Deadline First (EDF)** scheduling
- Tasks prioritized according to their deadlines
- Tie-breaking by lower Task ID
- Dynamic task-stack allocation
- Task sleeping and wake-up handling
- Periodic task support
- Runtime deadline modification
- Automatic context switches when a higher-priority deadline becomes active

Real-time APIs include:

```c
void osSleep(int timeInMs);
void osPeriodYield(void);
int osSetDeadline(int deadline, task_t TID);
int osCreateDeadlineTask(int deadline, TCB *task);
```

## Architecture

```text
+--------------------------------------------------+
|                User Applications                 |
+--------------------------------------------------+
                     |
                     | SVC
                     v
+--------------------------------------------------+
|                    RTX Kernel                    |
|                                                  |
|  +----------------+  +------------------------+  |
|  | Task Manager   |  | Memory Manager         |  |
|  |                |  |                        |  |
|  | - TCBs         |  | - First Fit Allocator  |  |
|  | - Task States  |  | - Free List            |  |
|  | - EDF Scheduler|  | - Coalescing           |  |
|  +----------------+  +------------------------+  |
|                                                  |
|  PendSV -> Context Switching                     |
|  SysTick -> Timing / Pre-emption                 |
+--------------------------------------------------+
                     |
                     v
+--------------------------------------------------+
|           ARM Cortex-M4 / STM32F4 MCU            |
+--------------------------------------------------+
```

## Context Switching

A context switch follows four main steps:

1. Save the currently running task's CPU context to its stack.
2. Run the scheduler to select the next task.
3. Restore the next task's saved context.
4. Resume execution from the task's previous location or entry point.

**PendSV** is used because it is designed by ARM for low-priority context-switch operations, allowing the kernel to separate scheduling logic from register save/restore operations.

## Memory Layout

The heap is placed in the unused RAM region between the end of the program image and the reserved system stack.

Conceptually:

```text
Low RAM Address
+----------------------------+
| Program Data / BSS         |
+----------------------------+
| _img_end                   |
+----------------------------+
|                            |
|        RTX Heap            |
|                            |
+----------------------------+
| Reserved Main Stack        |
+----------------------------+
| _estack                    |
+----------------------------+
High RAM Address
```

Each allocated block contains internal metadata followed by memory accessible to the requesting task.

## Scheduling Strategy

The kernel begins with **cooperative round-robin scheduling**, where tasks voluntarily call `osYield()`.

It is later upgraded to **pre-emptive EDF scheduling**:

```text
READY tasks
    |
    v
Compare remaining deadlines
    |
    v
Choose smallest deadline
    |
    +---- tie ----> choose lowest TID
    |
    v
RUNNING task
```

A task may be pre-empted when another task becomes eligible with an earlier deadline.

## Target Platform

- **Microcontroller:** STM32F401RE / STM32F411RE
- **CPU:** ARM Cortex-M4
- **Flash:** 512 KB
- **RAM:** 96 KB / 128 KB depending on MCU
- **Timing:** Cortex-M SysTick
- **Context Switching:** PendSV
- **Kernel Entry:** SVC
- **Debug / Programming:** ST-LINK / JTAG

## Repository Structure

```text
Real-time-Operating-System/
├── ece350_start/      # RTX source / STM32 project
├── feedback/          # Project feedback
└── README.md
```

The main implementation is contained inside `ece350_start/`.

## Design Goals

The project focuses on understanding how operating-system mechanisms work at a low level rather than relying on an existing RTOS.

Key concepts demonstrated include:

- **CPU context management**
- **Interrupts and exceptions**
- **Kernel/user separation**
- **Task scheduling**
- **Real-time deadlines**
- **Dynamic memory allocation**
- **Memory fragmentation**
- **Embedded systems programming**
- **ARM Cortex-M architecture**

## Technologies

`C` · `ARM Cortex-M4` · `STM32` · `Embedded Systems` · `RTOS` · `SVC` · `PendSV` · `SysTick` · `EDF Scheduling`

## Course Project

Developed as part of **ECE 350** at the **University of Waterloo**.

The project was built incrementally, with each stage extending the capabilities of the previous kernel implementation.

## Author

**Yohance Nayak**

GitHub: [Yohands123](https://github.com/Yohands123)
