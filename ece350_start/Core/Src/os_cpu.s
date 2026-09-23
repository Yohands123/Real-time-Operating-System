.syntax unified
.cpu cortex-m4
.thumb

.global SVC_Handler
.thumb_func
SVC_Handler:
    /*
     * Determine which stack pointer was active before entering SVC.
     *
     * LR bit 2:
     *   0 -> MSP was active
     *   1 -> PSP was active
     *
     * Pass the selected exception stack-frame pointer as argument R0 to
     * SVC_Handler_Main.
     */
    TST     LR, #4
    ITE     EQ
    MRSEQ   R0, MSP
    MRSNE   R0, PSP
    B       SVC_Handler_Main


.global PendSV_Handler
.thumb_func
PendSV_Handler:
    /*
     * Load current_tcb.
     *
     * current_tcb is NULL during the first context switch, so no previous
     * task context needs to be saved.
     */
    LDR     R0, =current_tcb
    LDR     R1, [R0]
    CBZ     R1, pend_sv_skip_save

    /*
     * Save the software-managed registers R4-R11 on the current task's PSP.
     *
     * The hardware has already saved R0-R3, R12, LR, PC, and xPSR.
     */
    MRS     R2, PSP
    STMDB   R2!, {R4-R11}

    /*
     * stack_ptr is at byte offset 20 in the TCB.
     */
    STR     R2, [R1, #20]

pend_sv_skip_save:
    /*
     * Select the next task. k_scheduler updates current_task and current_tcb.
     */
    BL      k_scheduler

    /*
     * Load the next task's saved PSP.
     */
    LDR     R0, =current_tcb
    LDR     R1, [R0]
    LDR     R2, [R1, #20]

    /*
     * Restore R4-R11 and update PSP. The processor restores the hardware
     * exception frame automatically when BX LR performs exception return.
     */
    LDMIA   R2!, {R4-R11}
    MSR     PSP, R2

    /*
     * 0xFFFFFFFD means:
     *   - return to Thread mode
     *   - use PSP
     *   - restore the basic exception frame
     */
    LDR     LR, =0xFFFFFFFD
    BX      LR