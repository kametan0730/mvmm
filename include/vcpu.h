#ifndef MVMM_VCPU_H
#define MVMM_VCPU_H

#include "types.h"
#include "vm.h"
#include "param.h"
#include "vgic.h"
#include "gic.h"

enum vcpu_state{
    UNUSED,
    CREATED,
    READY,
    RUNNING,
    IDLE,
};

struct vcpu{
    struct{
        u64 x[31];
        u64 spsr;
        u64 elr;
    } reg;
    struct{
        u64 spsr_el1;
        u64 elr_el1;
        u64 mpidr_el1;
        u64 midr_el1;
        u64 sp_el0;
        u64 sp_el1;
        u64 ttbr0_el1;
        u64 ttbr1_el1;
        u64 tcr_el1;
        u64 vbar_el1;
        u64 sctlr_el1;
        u64 cntv_ctl_el0;
        u64 cntv_tval_el0;
        u64 cntfrq_el0;
    } sys;

    struct gic_state gic;

    enum vcpu_state state;
    const char* name;
    struct vm* vm;
    struct vcpu* next;  /* ready queue */
    struct vgic_cpu* vgic;
    int cpuid;
};

extern struct vcpu vcpus[VCPU_MAX];

struct vcpu* new_vcpu(struct vm* vm, int vcpuid, u64 entry);

void free_vcpu(struct vcpu* vcpu);

void vcpu_ready(struct vcpu* vcpu);

void enter_vcpu(void);

#endif
