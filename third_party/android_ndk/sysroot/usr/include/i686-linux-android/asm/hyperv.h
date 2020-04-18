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
#ifndef _ASM_X86_HYPERV_H
#define _ASM_X86_HYPERV_H
#include <linux/types.h>
#define HYPERV_CPUID_VENDOR_AND_MAX_FUNCTIONS 0x40000000
#define HYPERV_CPUID_INTERFACE 0x40000001
#define HYPERV_CPUID_VERSION 0x40000002
#define HYPERV_CPUID_FEATURES 0x40000003
#define HYPERV_CPUID_ENLIGHTMENT_INFO 0x40000004
#define HYPERV_CPUID_IMPLEMENT_LIMITS 0x40000005
#define HYPERV_HYPERVISOR_PRESENT_BIT 0x80000000
#define HYPERV_CPUID_MIN 0x40000005
#define HYPERV_CPUID_MAX 0x4000ffff
#define HV_X64_MSR_VP_RUNTIME_AVAILABLE (1 << 0)
#define HV_X64_MSR_TIME_REF_COUNT_AVAILABLE (1 << 1)
#define HV_X64_MSR_REFERENCE_TSC_AVAILABLE (1 << 9)
#define HV_X64_MSR_REFERENCE_TSC 0x40000021
#define HV_X64_MSR_APIC_FREQUENCY_AVAILABLE (1 << 11)
#define HV_X64_MSR_TSC_FREQUENCY_AVAILABLE (1 << 11)
#define HV_X64_MSR_SYNIC_AVAILABLE (1 << 2)
#define HV_X64_MSR_SYNTIMER_AVAILABLE (1 << 3)
#define HV_X64_MSR_APIC_ACCESS_AVAILABLE (1 << 4)
#define HV_X64_MSR_HYPERCALL_AVAILABLE (1 << 5)
#define HV_X64_MSR_VP_INDEX_AVAILABLE (1 << 6)
#define HV_X64_MSR_RESET_AVAILABLE (1 << 7)
#define HV_X64_MSR_STAT_PAGES_AVAILABLE (1 << 8)
#define HV_FEATURE_GUEST_CRASH_MSR_AVAILABLE (1 << 10)
#define HV_X64_CREATE_PARTITIONS (1 << 0)
#define HV_X64_ACCESS_PARTITION_ID (1 << 1)
#define HV_X64_ACCESS_MEMORY_POOL (1 << 2)
#define HV_X64_ADJUST_MESSAGE_BUFFERS (1 << 3)
#define HV_X64_POST_MESSAGES (1 << 4)
#define HV_X64_SIGNAL_EVENTS (1 << 5)
#define HV_X64_CREATE_PORT (1 << 6)
#define HV_X64_CONNECT_PORT (1 << 7)
#define HV_X64_ACCESS_STATS (1 << 8)
#define HV_X64_DEBUGGING (1 << 11)
#define HV_X64_CPU_POWER_MANAGEMENT (1 << 12)
#define HV_X64_CONFIGURE_PROFILER (1 << 13)
#define HV_X64_MWAIT_AVAILABLE (1 << 0)
#define HV_X64_GUEST_DEBUGGING_AVAILABLE (1 << 1)
#define HV_X64_PERF_MONITOR_AVAILABLE (1 << 2)
#define HV_X64_CPU_DYNAMIC_PARTITIONING_AVAILABLE (1 << 3)
#define HV_X64_HYPERCALL_PARAMS_XMM_AVAILABLE (1 << 4)
#define HV_X64_GUEST_IDLE_STATE_AVAILABLE (1 << 5)
#define HV_X64_GUEST_CRASH_MSR_AVAILABLE (1 << 10)
#define HV_X64_AS_SWITCH_RECOMMENDED (1 << 0)
#define HV_X64_LOCAL_TLB_FLUSH_RECOMMENDED (1 << 1)
#define HV_X64_REMOTE_TLB_FLUSH_RECOMMENDED (1 << 2)
#define HV_X64_APIC_ACCESS_RECOMMENDED (1 << 3)
#define HV_X64_SYSTEM_RESET_RECOMMENDED (1 << 4)
#define HV_X64_RELAXED_TIMING_RECOMMENDED (1 << 5)
#define HV_X64_DEPRECATING_AEOI_RECOMMENDED (1 << 9)
#define HV_CRASH_CTL_CRASH_NOTIFY (1ULL << 63)
#define HV_X64_MSR_GUEST_OS_ID 0x40000000
#define HV_X64_MSR_HYPERCALL 0x40000001
#define HV_X64_MSR_VP_INDEX 0x40000002
#define HV_X64_MSR_RESET 0x40000003
#define HV_X64_MSR_VP_RUNTIME 0x40000010
#define HV_X64_MSR_TIME_REF_COUNT 0x40000020
#define HV_X64_MSR_TSC_FREQUENCY 0x40000022
#define HV_X64_MSR_APIC_FREQUENCY 0x40000023
#define HV_X64_MSR_EOI 0x40000070
#define HV_X64_MSR_ICR 0x40000071
#define HV_X64_MSR_TPR 0x40000072
#define HV_X64_MSR_APIC_ASSIST_PAGE 0x40000073
#define HV_X64_MSR_SCONTROL 0x40000080
#define HV_X64_MSR_SVERSION 0x40000081
#define HV_X64_MSR_SIEFP 0x40000082
#define HV_X64_MSR_SIMP 0x40000083
#define HV_X64_MSR_EOM 0x40000084
#define HV_X64_MSR_SINT0 0x40000090
#define HV_X64_MSR_SINT1 0x40000091
#define HV_X64_MSR_SINT2 0x40000092
#define HV_X64_MSR_SINT3 0x40000093
#define HV_X64_MSR_SINT4 0x40000094
#define HV_X64_MSR_SINT5 0x40000095
#define HV_X64_MSR_SINT6 0x40000096
#define HV_X64_MSR_SINT7 0x40000097
#define HV_X64_MSR_SINT8 0x40000098
#define HV_X64_MSR_SINT9 0x40000099
#define HV_X64_MSR_SINT10 0x4000009A
#define HV_X64_MSR_SINT11 0x4000009B
#define HV_X64_MSR_SINT12 0x4000009C
#define HV_X64_MSR_SINT13 0x4000009D
#define HV_X64_MSR_SINT14 0x4000009E
#define HV_X64_MSR_SINT15 0x4000009F
#define HV_X64_MSR_STIMER0_CONFIG 0x400000B0
#define HV_X64_MSR_STIMER0_COUNT 0x400000B1
#define HV_X64_MSR_STIMER1_CONFIG 0x400000B2
#define HV_X64_MSR_STIMER1_COUNT 0x400000B3
#define HV_X64_MSR_STIMER2_CONFIG 0x400000B4
#define HV_X64_MSR_STIMER2_COUNT 0x400000B5
#define HV_X64_MSR_STIMER3_CONFIG 0x400000B6
#define HV_X64_MSR_STIMER3_COUNT 0x400000B7
#define HV_X64_MSR_CRASH_P0 0x40000100
#define HV_X64_MSR_CRASH_P1 0x40000101
#define HV_X64_MSR_CRASH_P2 0x40000102
#define HV_X64_MSR_CRASH_P3 0x40000103
#define HV_X64_MSR_CRASH_P4 0x40000104
#define HV_X64_MSR_CRASH_CTL 0x40000105
#define HV_X64_MSR_CRASH_CTL_NOTIFY (1ULL << 63)
#define HV_X64_MSR_CRASH_PARAMS (1 + (HV_X64_MSR_CRASH_P4 - HV_X64_MSR_CRASH_P0))
#define HV_X64_MSR_HYPERCALL_ENABLE 0x00000001
#define HV_X64_MSR_HYPERCALL_PAGE_ADDRESS_SHIFT 12
#define HV_X64_MSR_HYPERCALL_PAGE_ADDRESS_MASK (~((1ull << HV_X64_MSR_HYPERCALL_PAGE_ADDRESS_SHIFT) - 1))
#define HVCALL_NOTIFY_LONG_SPIN_WAIT 0x0008
#define HVCALL_POST_MESSAGE 0x005c
#define HVCALL_SIGNAL_EVENT 0x005d
#define HV_X64_MSR_APIC_ASSIST_PAGE_ENABLE 0x00000001
#define HV_X64_MSR_APIC_ASSIST_PAGE_ADDRESS_SHIFT 12
#define HV_X64_MSR_APIC_ASSIST_PAGE_ADDRESS_MASK (~((1ull << HV_X64_MSR_APIC_ASSIST_PAGE_ADDRESS_SHIFT) - 1))
#define HV_X64_MSR_TSC_REFERENCE_ENABLE 0x00000001
#define HV_X64_MSR_TSC_REFERENCE_ADDRESS_SHIFT 12
#define HV_PROCESSOR_POWER_STATE_C0 0
#define HV_PROCESSOR_POWER_STATE_C1 1
#define HV_PROCESSOR_POWER_STATE_C2 2
#define HV_PROCESSOR_POWER_STATE_C3 3
#define HV_STATUS_SUCCESS 0
#define HV_STATUS_INVALID_HYPERCALL_CODE 2
#define HV_STATUS_INVALID_HYPERCALL_INPUT 3
#define HV_STATUS_INVALID_ALIGNMENT 4
#define HV_STATUS_INSUFFICIENT_MEMORY 11
#define HV_STATUS_INVALID_CONNECTION_ID 18
#define HV_STATUS_INSUFFICIENT_BUFFERS 19
typedef struct _HV_REFERENCE_TSC_PAGE {
  __u32 tsc_sequence;
  __u32 res1;
  __u64 tsc_scale;
  __s64 tsc_offset;
} HV_REFERENCE_TSC_PAGE, * PHV_REFERENCE_TSC_PAGE;
#define HV_SYNIC_SINT_COUNT (16)
#define HV_SYNIC_VERSION_1 (0x1)
#define HV_SYNIC_CONTROL_ENABLE (1ULL << 0)
#define HV_SYNIC_SIMP_ENABLE (1ULL << 0)
#define HV_SYNIC_SIEFP_ENABLE (1ULL << 0)
#define HV_SYNIC_SINT_MASKED (1ULL << 16)
#define HV_SYNIC_SINT_AUTO_EOI (1ULL << 17)
#define HV_SYNIC_SINT_VECTOR_MASK (0xFF)
#define HV_SYNIC_STIMER_COUNT (4)
#define HV_MESSAGE_SIZE (256)
#define HV_MESSAGE_PAYLOAD_BYTE_COUNT (240)
#define HV_MESSAGE_PAYLOAD_QWORD_COUNT (30)
enum hv_message_type {
  HVMSG_NONE = 0x00000000,
  HVMSG_UNMAPPED_GPA = 0x80000000,
  HVMSG_GPA_INTERCEPT = 0x80000001,
  HVMSG_TIMER_EXPIRED = 0x80000010,
  HVMSG_INVALID_VP_REGISTER_VALUE = 0x80000020,
  HVMSG_UNRECOVERABLE_EXCEPTION = 0x80000021,
  HVMSG_UNSUPPORTED_FEATURE = 0x80000022,
  HVMSG_EVENTLOG_BUFFERCOMPLETE = 0x80000040,
  HVMSG_X64_IOPORT_INTERCEPT = 0x80010000,
  HVMSG_X64_MSR_INTERCEPT = 0x80010001,
  HVMSG_X64_CPUID_INTERCEPT = 0x80010002,
  HVMSG_X64_EXCEPTION_INTERCEPT = 0x80010003,
  HVMSG_X64_APIC_EOI = 0x80010004,
  HVMSG_X64_LEGACY_FP_ERROR = 0x80010005
};
union hv_message_flags {
  __u8 asu8;
  struct {
    __u8 msg_pending : 1;
    __u8 reserved : 7;
  };
};
union hv_port_id {
  __u32 asu32;
  struct {
    __u32 id : 24;
    __u32 reserved : 8;
  } u;
};
struct hv_message_header {
  __u32 message_type;
  __u8 payload_size;
  union hv_message_flags message_flags;
  __u8 reserved[2];
  union {
    __u64 sender;
    union hv_port_id port;
  };
};
struct hv_message {
  struct hv_message_header header;
  union {
    __u64 payload[HV_MESSAGE_PAYLOAD_QWORD_COUNT];
  } u;
};
struct hv_message_page {
  struct hv_message sint_message[HV_SYNIC_SINT_COUNT];
};
struct hv_timer_message_payload {
  __u32 timer_index;
  __u32 reserved;
  __u64 expiration_time;
  __u64 delivery_time;
};
#define HV_STIMER_ENABLE (1ULL << 0)
#define HV_STIMER_PERIODIC (1ULL << 1)
#define HV_STIMER_LAZY (1ULL << 2)
#define HV_STIMER_AUTOENABLE (1ULL << 3)
#define HV_STIMER_SINT(config) (__u8) (((config) >> 16) & 0x0F)
#endif
