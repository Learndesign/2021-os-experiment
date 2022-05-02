#include <asm.h>
#include <csr.h>

/*.macro定义宏定义，SAVE_CONTEXT*/
/*保存上下文*/
.macro SAVE_CONTEXT
  /*定义这个标号只能在改文件内使用*/
  sd sp, PCB_USER_SP(tp)      /*来自userspace，切换到内核，保存用户sp*/
  ld sp, PCB_KERNEL_SP(tp)    /*load内核sp*/
  addi sp, sp, SWITCH_TO_SIZE /*sp向后移动288为所有寄存器提供空间*/

  /* TODO: save all general purpose registers here! */
  sd ra, OFFSET_REG_RA(sp)
  sd gp, OFFSET_REG_GP(sp)
  sd tp, OFFSET_REG_TP(sp)
  sd t0, OFFSET_REG_T0(sp)
  sd t1, OFFSET_REG_T1(sp)
  sd t2, OFFSET_REG_T2(sp)
  sd s0, OFFSET_REG_S0(sp)
  sd s1, OFFSET_REG_S1(sp)
  sd a0, OFFSET_REG_A0(sp)
  sd a1, OFFSET_REG_A1(sp)
  sd a2, OFFSET_REG_A2(sp)
  sd a3, OFFSET_REG_A3(sp)
  sd a4, OFFSET_REG_A4(sp)
  sd a5, OFFSET_REG_A5(sp)
  sd a6, OFFSET_REG_A6(sp)
  sd a7, OFFSET_REG_A7(sp)
  sd s2, OFFSET_REG_S2(sp)
  sd s3, OFFSET_REG_S3(sp)
  sd s4, OFFSET_REG_S4(sp)
  sd s5, OFFSET_REG_S5(sp)
  sd s6, OFFSET_REG_S6(sp)
  sd s7, OFFSET_REG_S7(sp)
  sd s8, OFFSET_REG_S8(sp)
  sd s9, OFFSET_REG_S9(sp)
  sd s10, OFFSET_REG_S10(sp)
  sd s11, OFFSET_REG_S11(sp)
  sd t3, OFFSET_REG_T3(sp)
  sd t4, OFFSET_REG_T4(sp)
  sd t5, OFFSET_REG_T5(sp)
  sd t6, OFFSET_REG_T6(sp)
  /*恢复用户指针*/
  ld t0, PCB_USER_SP(tp)
  sd t0, OFFSET_REG_SP(sp)
  /*
   * Disable user-mode memory access as it should only be set in the
   * actual user copy routines.
   *
   * Disable the FPU（浮点运算单元） to detect illegal usage of floating point in kernel
   * space.
   */
  li t0, /*SR_SUM |*/ SR_FS //S态
  /* TODO: save sstatus, sepc, stval, scause and sscratch on user stack */
  /* 
   * 状态控制寄存器没有直接写内存的指令模式，需要先装入寄存器再写
   * 此时所有的寄存器都已经保存，故可以使用寄存器操作
   * 个别寄存器
  */
  csrr t0 ,CSR_SSTATUS
  sd   t0 ,OFFSET_REG_SSTATUS(sp)
  csrr t0 ,CSR_SEPC         
  sd   t0 ,OFFSET_REG_SEPC(sp)
  csrr t0 ,CSR_STVAL
  sd   t0 ,OFFSET_REG_SBADADDR(sp)
  csrr t0 ,CSR_SCAUSE
  sd   t0 ,OFFSET_REG_SCAUSE(sp)
  /*
   * 保护现场意味着已经进入内核态，将栈设置为内核栈
  */
  ld   sp ,PCB_KERNEL_SP(tp) 
.endm

/*恢复上下文*/
.macro RESTORE_CONTEXT
  /* TODO: Restore all registers and sepc,sstatus */
  /*恢复S态下的控制状态寄存器*/
  ld   sp, PCB_KERNEL_SP(tp)
  addi sp, sp, SWITCH_TO_SIZE
  ld   s0, OFFSET_REG_SSTATUS(sp)
  ld   s1, OFFSET_REG_SEPC(sp)
  ld   s2, OFFSET_REG_SBADADDR(sp)
  ld   s3, OFFSET_REG_SCAUSE(sp)
  csrw CSR_SSTATUS, s0
  csrw CSR_SEPC, s1
  csrw CSR_STVAL, s2
  csrw CSR_SCAUSE, s3 
  /*恢复32个普通寄存器*/
  ld ra, OFFSET_REG_RA(sp)    
  ld gp, OFFSET_REG_GP(sp)
  ld tp, OFFSET_REG_TP(sp)
  ld t0, OFFSET_REG_T0(sp)
  ld t1, OFFSET_REG_T1(sp)
  ld t2, OFFSET_REG_T2(sp)
  ld s0, OFFSET_REG_S0(sp)
  ld s1, OFFSET_REG_S1(sp)
  ld a0, OFFSET_REG_A0(sp)
  ld a1, OFFSET_REG_A1(sp)
  ld a2, OFFSET_REG_A2(sp)
  ld a3, OFFSET_REG_A3(sp)
  ld a4, OFFSET_REG_A4(sp)
  ld a5, OFFSET_REG_A5(sp)
  ld a6, OFFSET_REG_A6(sp)
  ld a7, OFFSET_REG_A7(sp)
  ld s2, OFFSET_REG_S2(sp)
  ld s3, OFFSET_REG_S3(sp)
  ld s4, OFFSET_REG_S4(sp)
  ld s5, OFFSET_REG_S5(sp)
  ld s6, OFFSET_REG_S6(sp)
  ld s7, OFFSET_REG_S7(sp)
  ld s8, OFFSET_REG_S8(sp)
  ld s9, OFFSET_REG_S9(sp)
  ld s10, OFFSET_REG_S10(sp)
  ld s11, OFFSET_REG_S11(sp)
  ld t3, OFFSET_REG_T3(sp)
  ld t4, OFFSET_REG_T4(sp)
  ld t5, OFFSET_REG_T5(sp)
  ld t6, OFFSET_REG_T6(sp)
  /*恢复现场意味着将要进入用户态，进入到用户栈运行*/
  ld sp, OFFSET_REG_SP(sp)
.endm

ENTRY(enable_preempt)
//   ld t1, current_running
//   ld t0, PCB_PREEMPT_COUNT(t1)
//   beq t0, zero, do_enable
//   addi t0, t0, -1
//   sd t0, PCB_PREEMPT_COUNT(t1)
//   beq t0, zero, do_enable
//   jr ra
// do_enable:
//   not t0, x0
//   csrs CSR_SIE, t0
  jr ra
ENDPROC(enable_preempt)

ENTRY(disable_preempt)
//  csrw CSR_SIE, zero
//  ld t1, current_running
//  ld t0, PCB_PREEMPT_COUNT(t1)
//  addi t0, t0, 1
//  sd t0, PCB_PREEMPT_COUNT(t1)
  jr ra
ENDPROC(disable_preempt)

/*开中断*/
ENTRY(enable_interrupt)
  li t0, SR_SIE
  csrs CSR_SSTATUS, t0
  jr ra
ENDPROC(enable_interrupt)

/*关中断*/
ENTRY(disable_interrupt)
  li t0, SR_SIE
  csrc CSR_SSTATUS, t0
  jr ra
ENDPROC(disable_interrupt)

// the address of previous pcb in a0
// the address of next pcb in a1
ENTRY(switch_to)
  ld t0, PCB_KERNEL_SP(a0)
  sd sp, SWITCH_TO_SP(t0)
  ld sp, PCB_KERNEL_SP(a0)
  /* TODO: store all callee save registers,
   * see the definition of `struct switchto_context` in sched.h*/
  sd ra, SWITCH_TO_RA(sp)
  sd s0, SWITCH_TO_S0(sp)
  sd s1, SWITCH_TO_S1(sp)
  sd s2, SWITCH_TO_S2(sp)
  sd s3, SWITCH_TO_S3(sp)
  sd s4, SWITCH_TO_S4(sp)
  sd s5, SWITCH_TO_S5(sp)
  sd s6, SWITCH_TO_S6(sp)
  sd s7, SWITCH_TO_S7(sp)
  sd s8, SWITCH_TO_S8(sp)
  sd s9, SWITCH_TO_S9(sp)
  sd s10, SWITCH_TO_S10(sp)
  sd s11, SWITCH_TO_S11(sp)

  // restore next
  /* TODO: restore all callee save registers,
   * see the definition of `struct switchto_context` in sched.h*/
  ld sp, PCB_KERNEL_SP(a1)
  ld ra, SWITCH_TO_RA(sp)
  ld s0, SWITCH_TO_S0(sp)
  ld s1, SWITCH_TO_S1(sp)
  ld s2, SWITCH_TO_S2(sp)
  ld s3, SWITCH_TO_S3(sp)
  ld s4, SWITCH_TO_S4(sp)
  ld s5, SWITCH_TO_S5(sp)
  ld s6, SWITCH_TO_S6(sp)
  ld s7, SWITCH_TO_S7(sp)
  ld s8, SWITCH_TO_S8(sp)
  ld s9, SWITCH_TO_S9(sp)
  ld s10, SWITCH_TO_S10(sp)
  ld s11, SWITCH_TO_S11(sp)
  ld sp, SWITCH_TO_SP(sp)
  mv tp, a1
  /*task1,2,ra为entry_point，task3,4,ra为ret_from_exception*/
  jr ra
ENDPROC(switch_to)

ENTRY(ret_from_exception)
  /* TODO: */
  csrw CSR_SSCRATCH,tp
  
  /*解锁*/
  call unlock_kernel  
  /*恢复现场*/
  RESTORE_CONTEXT
  sret  
ENDPROC(ret_from_exception)

ENTRY(exception_handler_entry)
  SAVE_CONTEXT

  // csrw CSR_SSCRATCH, x0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  /*上锁*/
  call lock_kernel
  /* TODO: load ret_from_exception into $ra
   * so that we can return to ret_from_exception
   * when interrupt_help complete.
   */
  la ra, ret_from_exception
  /* TODO: call interrupt_helper
   * note: don't forget to pass parameters for it.
   */
  ld a0, PCB_KERNEL_SP(tp)
  addi a0, a0, SWITCH_TO_SIZE
  csrr a1, CSR_STVAL
  csrr a2, CSR_SCAUSE
  la t0, interrupt_helper
  jr t0
ENDPROC(exception_handler_entry)
