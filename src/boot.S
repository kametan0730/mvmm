.section ".text.boot"

.global _start
_start:
  mrs x1, mpidr_el1
  and x1, x1, #3
  cbz x1, vmm_start

vmm_start:
  adrp x0, _stack
  add x0, x0, #4096
  mov sp, x0
  bl vmm_init
  b .
