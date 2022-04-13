#ifndef MVMM_VM_H
#define MVMM_VM_H

#include "types.h"
#include "param.h"
#include "vgic.h"

struct mmio_info;
struct vcpu;
struct vm{
    char name[16];
    int nvcpu;
    struct vcpu* vcpus[VCPU_MAX];
    u64* vttbr;
    struct vgic* vgic;
    struct mmio_info* pmap;
    int npmap;
    u64 entry;
    int used;
};

extern struct vm vms[VM_MAX];

void new_vm(char* name, int ncpu, u64 img_start, u64 img_size, u64 entry, u64 allocated);

#endif
