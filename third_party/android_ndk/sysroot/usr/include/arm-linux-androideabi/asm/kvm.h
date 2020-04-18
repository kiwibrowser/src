/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef __ARM_KVM_H__
#define __ARM_KVM_H__
#include <linux/types.h>
#include <linux/psci.h>
#include <asm/ptrace.h>
#define __KVM_HAVE_GUEST_DEBUG
#define __KVM_HAVE_IRQ_LINE
#define __KVM_HAVE_READONLY_MEM
#define KVM_COALESCED_MMIO_PAGE_OFFSET 1
#define KVM_REG_SIZE(id) (1U << (((id) & KVM_REG_SIZE_MASK) >> KVM_REG_SIZE_SHIFT))
#define KVM_ARM_SVC_sp svc_regs[0]
#define KVM_ARM_SVC_lr svc_regs[1]
#define KVM_ARM_SVC_spsr svc_regs[2]
#define KVM_ARM_ABT_sp abt_regs[0]
#define KVM_ARM_ABT_lr abt_regs[1]
#define KVM_ARM_ABT_spsr abt_regs[2]
#define KVM_ARM_UND_sp und_regs[0]
#define KVM_ARM_UND_lr und_regs[1]
#define KVM_ARM_UND_spsr und_regs[2]
#define KVM_ARM_IRQ_sp irq_regs[0]
#define KVM_ARM_IRQ_lr irq_regs[1]
#define KVM_ARM_IRQ_spsr irq_regs[2]
#define KVM_ARM_FIQ_r8 fiq_regs[0]
#define KVM_ARM_FIQ_r9 fiq_regs[1]
#define KVM_ARM_FIQ_r10 fiq_regs[2]
#define KVM_ARM_FIQ_fp fiq_regs[3]
#define KVM_ARM_FIQ_ip fiq_regs[4]
#define KVM_ARM_FIQ_sp fiq_regs[5]
#define KVM_ARM_FIQ_lr fiq_regs[6]
#define KVM_ARM_FIQ_spsr fiq_regs[7]
struct kvm_regs {
  struct pt_regs usr_regs;
  unsigned long svc_regs[3];
  unsigned long abt_regs[3];
  unsigned long und_regs[3];
  unsigned long irq_regs[3];
  unsigned long fiq_regs[8];
};
#define KVM_ARM_TARGET_CORTEX_A15 0
#define KVM_ARM_TARGET_CORTEX_A7 1
#define KVM_ARM_NUM_TARGETS 2
#define KVM_ARM_DEVICE_TYPE_SHIFT 0
#define KVM_ARM_DEVICE_TYPE_MASK (0xffff << KVM_ARM_DEVICE_TYPE_SHIFT)
#define KVM_ARM_DEVICE_ID_SHIFT 16
#define KVM_ARM_DEVICE_ID_MASK (0xffff << KVM_ARM_DEVICE_ID_SHIFT)
#define KVM_ARM_DEVICE_VGIC_V2 0
#define KVM_VGIC_V2_ADDR_TYPE_DIST 0
#define KVM_VGIC_V2_ADDR_TYPE_CPU 1
#define KVM_VGIC_V2_DIST_SIZE 0x1000
#define KVM_VGIC_V2_CPU_SIZE 0x2000
#define KVM_VGIC_V3_ADDR_TYPE_DIST 2
#define KVM_VGIC_V3_ADDR_TYPE_REDIST 3
#define KVM_VGIC_ITS_ADDR_TYPE 4
#define KVM_VGIC_V3_DIST_SIZE SZ_64K
#define KVM_VGIC_V3_REDIST_SIZE (2 * SZ_64K)
#define KVM_VGIC_V3_ITS_SIZE (2 * SZ_64K)
#define KVM_ARM_VCPU_POWER_OFF 0
#define KVM_ARM_VCPU_PSCI_0_2 1
struct kvm_vcpu_init {
  __u32 target;
  __u32 features[7];
};
struct kvm_sregs {
};
struct kvm_fpu {
};
struct kvm_guest_debug_arch {
};
struct kvm_debug_exit_arch {
};
struct kvm_sync_regs {
  __u64 device_irq_level;
};
struct kvm_arch_memory_slot {
};
#define KVM_REG_ARM_COPROC_MASK 0x000000000FFF0000
#define KVM_REG_ARM_COPROC_SHIFT 16
#define KVM_REG_ARM_32_OPC2_MASK 0x0000000000000007
#define KVM_REG_ARM_32_OPC2_SHIFT 0
#define KVM_REG_ARM_OPC1_MASK 0x0000000000000078
#define KVM_REG_ARM_OPC1_SHIFT 3
#define KVM_REG_ARM_CRM_MASK 0x0000000000000780
#define KVM_REG_ARM_CRM_SHIFT 7
#define KVM_REG_ARM_32_CRN_MASK 0x0000000000007800
#define KVM_REG_ARM_32_CRN_SHIFT 11
#define ARM_CP15_REG_SHIFT_MASK(x,n) (((x) << KVM_REG_ARM_ ##n ##_SHIFT) & KVM_REG_ARM_ ##n ##_MASK)
#define __ARM_CP15_REG(op1,crn,crm,op2) (KVM_REG_ARM | (15 << KVM_REG_ARM_COPROC_SHIFT) | ARM_CP15_REG_SHIFT_MASK(op1, OPC1) | ARM_CP15_REG_SHIFT_MASK(crn, 32_CRN) | ARM_CP15_REG_SHIFT_MASK(crm, CRM) | ARM_CP15_REG_SHIFT_MASK(op2, 32_OPC2))
#define ARM_CP15_REG32(...) (__ARM_CP15_REG(__VA_ARGS__) | KVM_REG_SIZE_U32)
#define __ARM_CP15_REG64(op1,crm) (__ARM_CP15_REG(op1, 0, crm, 0) | KVM_REG_SIZE_U64)
#define ARM_CP15_REG64(...) __ARM_CP15_REG64(__VA_ARGS__)
#define KVM_REG_ARM_TIMER_CTL ARM_CP15_REG32(0, 14, 3, 1)
#define KVM_REG_ARM_TIMER_CNT ARM_CP15_REG64(1, 14)
#define KVM_REG_ARM_TIMER_CVAL ARM_CP15_REG64(3, 14)
#define KVM_REG_ARM_CORE (0x0010 << KVM_REG_ARM_COPROC_SHIFT)
#define KVM_REG_ARM_CORE_REG(name) (offsetof(struct kvm_regs, name) / 4)
#define KVM_REG_ARM_DEMUX (0x0011 << KVM_REG_ARM_COPROC_SHIFT)
#define KVM_REG_ARM_DEMUX_ID_MASK 0x000000000000FF00
#define KVM_REG_ARM_DEMUX_ID_SHIFT 8
#define KVM_REG_ARM_DEMUX_ID_CCSIDR (0x00 << KVM_REG_ARM_DEMUX_ID_SHIFT)
#define KVM_REG_ARM_DEMUX_VAL_MASK 0x00000000000000FF
#define KVM_REG_ARM_DEMUX_VAL_SHIFT 0
#define KVM_REG_ARM_VFP (0x0012 << KVM_REG_ARM_COPROC_SHIFT)
#define KVM_REG_ARM_VFP_MASK 0x000000000000FFFF
#define KVM_REG_ARM_VFP_BASE_REG 0x0
#define KVM_REG_ARM_VFP_FPSID 0x1000
#define KVM_REG_ARM_VFP_FPSCR 0x1001
#define KVM_REG_ARM_VFP_MVFR1 0x1006
#define KVM_REG_ARM_VFP_MVFR0 0x1007
#define KVM_REG_ARM_VFP_FPEXC 0x1008
#define KVM_REG_ARM_VFP_FPINST 0x1009
#define KVM_REG_ARM_VFP_FPINST2 0x100A
#define KVM_DEV_ARM_VGIC_GRP_ADDR 0
#define KVM_DEV_ARM_VGIC_GRP_DIST_REGS 1
#define KVM_DEV_ARM_VGIC_GRP_CPU_REGS 2
#define KVM_DEV_ARM_VGIC_CPUID_SHIFT 32
#define KVM_DEV_ARM_VGIC_CPUID_MASK (0xffULL << KVM_DEV_ARM_VGIC_CPUID_SHIFT)
#define KVM_DEV_ARM_VGIC_V3_MPIDR_SHIFT 32
#define KVM_DEV_ARM_VGIC_V3_MPIDR_MASK (0xffffffffULL << KVM_DEV_ARM_VGIC_V3_MPIDR_SHIFT)
#define KVM_DEV_ARM_VGIC_OFFSET_SHIFT 0
#define KVM_DEV_ARM_VGIC_OFFSET_MASK (0xffffffffULL << KVM_DEV_ARM_VGIC_OFFSET_SHIFT)
#define KVM_DEV_ARM_VGIC_SYSREG_INSTR_MASK (0xffff)
#define KVM_DEV_ARM_VGIC_GRP_NR_IRQS 3
#define KVM_DEV_ARM_VGIC_GRP_CTRL 4
#define KVM_DEV_ARM_VGIC_GRP_REDIST_REGS 5
#define KVM_DEV_ARM_VGIC_GRP_CPU_SYSREGS 6
#define KVM_DEV_ARM_VGIC_GRP_LEVEL_INFO 7
#define KVM_DEV_ARM_VGIC_GRP_ITS_REGS 8
#define KVM_DEV_ARM_VGIC_LINE_LEVEL_INFO_SHIFT 10
#define KVM_DEV_ARM_VGIC_LINE_LEVEL_INFO_MASK (0x3fffffULL << KVM_DEV_ARM_VGIC_LINE_LEVEL_INFO_SHIFT)
#define KVM_DEV_ARM_VGIC_LINE_LEVEL_INTID_MASK 0x3ff
#define VGIC_LEVEL_INFO_LINE_LEVEL 0
#define KVM_DEV_ARM_VGIC_CTRL_INIT 0
#define KVM_DEV_ARM_ITS_SAVE_TABLES 1
#define KVM_DEV_ARM_ITS_RESTORE_TABLES 2
#define KVM_DEV_ARM_VGIC_SAVE_PENDING_TABLES 3
#define KVM_ARM_IRQ_TYPE_SHIFT 24
#define KVM_ARM_IRQ_TYPE_MASK 0xff
#define KVM_ARM_IRQ_VCPU_SHIFT 16
#define KVM_ARM_IRQ_VCPU_MASK 0xff
#define KVM_ARM_IRQ_NUM_SHIFT 0
#define KVM_ARM_IRQ_NUM_MASK 0xffff
#define KVM_ARM_IRQ_TYPE_CPU 0
#define KVM_ARM_IRQ_TYPE_SPI 1
#define KVM_ARM_IRQ_TYPE_PPI 2
#define KVM_ARM_IRQ_CPU_IRQ 0
#define KVM_ARM_IRQ_CPU_FIQ 1
#define KVM_ARM_IRQ_GIC_MAX 127
#define KVM_NR_IRQCHIPS 1
#define KVM_PSCI_FN_BASE 0x95c1ba5e
#define KVM_PSCI_FN(n) (KVM_PSCI_FN_BASE + (n))
#define KVM_PSCI_FN_CPU_SUSPEND KVM_PSCI_FN(0)
#define KVM_PSCI_FN_CPU_OFF KVM_PSCI_FN(1)
#define KVM_PSCI_FN_CPU_ON KVM_PSCI_FN(2)
#define KVM_PSCI_FN_MIGRATE KVM_PSCI_FN(3)
#define KVM_PSCI_RET_SUCCESS PSCI_RET_SUCCESS
#define KVM_PSCI_RET_NI PSCI_RET_NOT_SUPPORTED
#define KVM_PSCI_RET_INVAL PSCI_RET_INVALID_PARAMS
#define KVM_PSCI_RET_DENIED PSCI_RET_DENIED
#endif
