#include <asm.h>
ENTRY(sys_fork)
  mv a7, a0
  mv a0, a1
  mv a1, a2
  mv a2, a3
  ecall
  jr ra
ENDPROC(sys_fork)