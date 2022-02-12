#include "types.h"
#include "vm.h"
#include "vcpu.h"
#include "mm.h"
#include "pmalloc.h"
#include "lib.h"
#include "printf.h"
#include "memmap.h"

struct vm vms[VM_MAX];

static struct vm *allocvm() {
  for(struct vm *vm = vms; vm < &vms[VM_MAX]; vm++) {
    if(vm->used == 0) {
      vm->used = 1;
      return vm;
    }
  }

  return NULL;
}

void new_vm(char *name, int ncpu, u64 img_start, u64 img_size, u64 entry, u64 allocated) {
  printf("new VM\n");

  if(img_size > allocated)
    panic("img_size > allocated");
  if(allocated % PAGESIZE != 0)
    panic("mem align");

  struct vm *vm = allocvm();

  strcpy(vm->name, name);
  struct vcpu *vtmp[ncpu];

  for(int i = 0; i < ncpu; i++)
    vtmp[i] = new_vcpu(vm, i, entry);

  vm->entry = entry;

  u64 *vttbr = pmalloc();
  if(!vttbr)
    panic("vttbr");

  u64 p, cpsize;
  for(p = 0; p < img_size; p += PAGESIZE) {
    char *page = pmalloc();
    if(!page)
      panic("img");

    if(img_size - p > PAGESIZE)
      cpsize = PAGESIZE;
    else
      cpsize = img_size - p;
    memcpy(page, (char *)img_start+p, cpsize);
    pagemap(vttbr, entry+p, (u64)page, PAGESIZE, S2PTE_NORMAL);
  }

  for(; p < allocated; p += PAGESIZE) {
    char *page = pmalloc();
    if(!page)
      panic("ram");
    pagemap(vttbr, entry+p, (u64)page, PAGESIZE, S2PTE_NORMAL);
  }

  pagemap(vttbr, UARTBASE, UARTBASE, PAGESIZE, S2PTE_DEVICE);

  vm->vttbr = vttbr;

  for(int i = 0; i < ncpu; i++)
    vtmp[i]->state = READY;
}
