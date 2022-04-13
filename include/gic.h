#ifndef MVMM_GIC_H
#define MVMM_GIC_H

#include "types.h"
#include "memmap.h"

#define GIC_NSGI     16
#define GIC_SGI_MAX  15
#define GIC_NPPI     16
#define GIC_PPI_MAX  31

#define is_sgi(intid)     (0 <= (intid) && (intid) < 16)
#define is_ppi(intid)     (16 <= (intid) && (intid) < 32)
#define is_sgi_ppi(intid) (is_sgi(intid) || is_ppi(intid))
#define is_spi(intid)     (32 <= (intid))

#define ich_hcr_el2   arm_sysreg(4, c12, c11, 0)
#define ich_vtr_el2   arm_sysreg(4, c12, c11, 1)
#define ich_vmcr_el2  arm_sysreg(4, c12, c11, 7)
#define ich_lr0_el2   arm_sysreg(4, c12, c12, 0)
#define ich_lr1_el2   arm_sysreg(4, c12, c12, 1)
#define ich_lr2_el2   arm_sysreg(4, c12, c12, 2)
#define ich_lr3_el2   arm_sysreg(4, c12, c12, 3)
#define ich_lr4_el2   arm_sysreg(4, c12, c12, 4)
#define ich_lr5_el2   arm_sysreg(4, c12, c12, 5)
#define ich_lr6_el2   arm_sysreg(4, c12, c12, 6)
#define ich_lr7_el2   arm_sysreg(4, c12, c12, 7)
#define ich_lr8_el2   arm_sysreg(4, c12, c13, 0)
#define ich_lr9_el2   arm_sysreg(4, c12, c13, 1)
#define ich_lr10_el2  arm_sysreg(4, c12, c13, 2)
#define ich_lr11_el2  arm_sysreg(4, c12, c13, 3)
#define ich_lr12_el2  arm_sysreg(4, c12, c13, 4)
#define ich_lr13_el2  arm_sysreg(4, c12, c13, 5)
#define ich_lr14_el2  arm_sysreg(4, c12, c13, 6)
#define ich_lr15_el2  arm_sysreg(4, c12, c13, 7)

#define icc_pmr_el1       arm_sysreg(0, c4, c6, 0)
#define icc_eoir0_el1     arm_sysreg(0, c12, c8, 1)
#define icc_dir_el1       arm_sysreg(0, c12, c11, 1)
#define icc_iar1_el1      arm_sysreg(0, c12, c12, 0)
#define icc_eoir1_el1     arm_sysreg(0, c12, c12, 1)
#define icc_ctlr_el1      arm_sysreg(0, c12, c12, 4)
#define icc_sre_el1       arm_sysreg(0, c12, c12, 5)
#define icc_igrpen0_el1   arm_sysreg(0, c12, c12, 6)
#define icc_igrpen1_el1   arm_sysreg(0, c12, c12, 7)

#define icc_sre_el2       arm_sysreg(4, c12, c9, 5)

#define ICC_CTLR_EOImode(m) ((m) << 1)

#define ICH_HCR_EN  (1<<0)

#define ICH_VMCR_VENG0  (1<<0)
#define ICH_VMCR_VENG1  (1<<1)

#define ICH_LR_VINTID(n)   ((n) & 0xffffffffL)
#define ICH_LR_PINTID(n)   (((n) & 0x3ffL) << 32)
#define ICH_LR_GROUP(n)    (((n) & 0x1L) << 60)
#define ICH_LR_HW          (1L << 61)
#define ICH_LR_STATE(n)    (((n) & 0x3L) << 62)
#define LR_INACTIVE  0L
#define LR_PENDING   1L
#define LR_ACTIVE    2L
#define LR_PENDACT   3L
#define LR_MASK      3L

#define lr_is_inactive(lr)  (((lr >> 62) & 0x3) == LR_INACTIVE)
#define lr_is_pending(lr)   (((lr >> 62) & 0x3) == LR_PENDING)
#define lr_is_active(lr)    (((lr >> 62) & 0x3) == LR_ACTIVE)
#define lr_is_pendact(lr)   (((lr >> 62) & 0x3) == LR_PENDACT)

#define GICD_CTLR           (0x0)
#define GICD_CTLR_ENGRP(grp)    (1<<(grp))

#define GICD_TYPER          (0x4)
#define GICD_IGROUPR(n)     (0x80 + (u64)(n) * 4)
#define GICD_ISENABLER(n)   (0x100 + (u64)(n) * 4)
#define GICD_ICENABLER(n)   (0x180 + (u64)(n) * 4)
#define GICD_ISPENDR(n)     (0x200 + (u64)(n) * 4)
#define GICD_ICPENDR(n)     (0x280 + (u64)(n) * 4)
#define GICD_IPRIORITYR(n)  (0x400 + (u64)(n) * 4)
#define GICD_ITARGETSR(n)   (0x800 + (u64)(n) * 4)
#define GICD_ICFGR(n)       (0xc00 + (u64)(n) * 4)

#define GICRBASEn(n)        (GICRBASE+(n)*0x20000)

#define GICR_CTLR           (0x0)
#define GICR_WAKER          (0x14)

#define SGI_BASE  0x10000
#define GICR_IGROUPR0       (SGI_BASE+0x80)
#define GICR_ISENABLER0     (SGI_BASE+0x100)
#define GICR_ICENABLER0     (SGI_BASE+0x180)
#define GICR_ICPENDR0       (SGI_BASE+0x280)
#define GICR_IPRIORITYR(n)  (SGI_BASE+0x400+(n)*4)
#define GICR_ICFGR0         (SGI_BASE+0xc00)
#define GICR_ICFGR1         (SGI_BASE+0xc04)
#define GICR_IGRPMODR0      (SGI_BASE+0xd00)

static inline u32 gicd_r(u32 offset){
    return *(volatile u32*) (u64) (GICDBASE + offset);
}

static inline void gicd_w(u32 offset, u32 val){
    *(volatile u32*) (u64) (GICDBASE + offset) = val;
}

static inline u32 gicr_r32(int cpuid, u32 offset){
    return *(volatile u32*) (u64) (GICRBASEn(cpuid) + offset);
}

static inline void gicr_w32(int cpuid, u32 offset, u32 val){
    *(volatile u32*) (u64) (GICRBASEn(cpuid) + offset) = val;
}

struct gic_state{
    u64 lr[16];
    u64 vmcr;
    u32 sre_el1;
};

void gic_init(void);

void gic_init_cpu(int cpuid);

u32 gic_read_iar(void);

void gic_eoi(u32 iar, int grp);

void gic_deactive_int(u32 irq);

int gic_max_spi(void);

void gic_set_target(u32 irq, u8 target);

u64 gic_read_lr(int n);

void gic_write_lr(int n, u64 val);

u64 gic_make_lr(u32 pirq, u32 virq, int grp);

void gic_irq_enable(u32 irq);

void gic_irq_disable(u32 irq);

void gic_irq_enable_redist(u32 cpuid, u32 irq);

void gic_restore_state(struct gic_state* gic);

void gic_init_state(struct gic_state* gic);

#endif
