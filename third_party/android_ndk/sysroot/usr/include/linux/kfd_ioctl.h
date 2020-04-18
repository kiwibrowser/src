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
#ifndef KFD_IOCTL_H_INCLUDED
#define KFD_IOCTL_H_INCLUDED
#include <linux/types.h>
#include <linux/ioctl.h>
#define KFD_IOCTL_MAJOR_VERSION 1
#define KFD_IOCTL_MINOR_VERSION 1
struct kfd_ioctl_get_version_args {
  uint32_t major_version;
  uint32_t minor_version;
};
#define KFD_IOC_QUEUE_TYPE_COMPUTE 0
#define KFD_IOC_QUEUE_TYPE_SDMA 1
#define KFD_IOC_QUEUE_TYPE_COMPUTE_AQL 2
#define KFD_MAX_QUEUE_PERCENTAGE 100
#define KFD_MAX_QUEUE_PRIORITY 15
struct kfd_ioctl_create_queue_args {
  uint64_t ring_base_address;
  uint64_t write_pointer_address;
  uint64_t read_pointer_address;
  uint64_t doorbell_offset;
  uint32_t ring_size;
  uint32_t gpu_id;
  uint32_t queue_type;
  uint32_t queue_percentage;
  uint32_t queue_priority;
  uint32_t queue_id;
  uint64_t eop_buffer_address;
  uint64_t eop_buffer_size;
  uint64_t ctx_save_restore_address;
  uint64_t ctx_save_restore_size;
};
struct kfd_ioctl_destroy_queue_args {
  uint32_t queue_id;
  uint32_t pad;
};
struct kfd_ioctl_update_queue_args {
  uint64_t ring_base_address;
  uint32_t queue_id;
  uint32_t ring_size;
  uint32_t queue_percentage;
  uint32_t queue_priority;
};
#define KFD_IOC_CACHE_POLICY_COHERENT 0
#define KFD_IOC_CACHE_POLICY_NONCOHERENT 1
struct kfd_ioctl_set_memory_policy_args {
  uint64_t alternate_aperture_base;
  uint64_t alternate_aperture_size;
  uint32_t gpu_id;
  uint32_t default_policy;
  uint32_t alternate_policy;
  uint32_t pad;
};
struct kfd_ioctl_get_clock_counters_args {
  uint64_t gpu_clock_counter;
  uint64_t cpu_clock_counter;
  uint64_t system_clock_counter;
  uint64_t system_clock_freq;
  uint32_t gpu_id;
  uint32_t pad;
};
#define NUM_OF_SUPPORTED_GPUS 7
struct kfd_process_device_apertures {
  uint64_t lds_base;
  uint64_t lds_limit;
  uint64_t scratch_base;
  uint64_t scratch_limit;
  uint64_t gpuvm_base;
  uint64_t gpuvm_limit;
  uint32_t gpu_id;
  uint32_t pad;
};
struct kfd_ioctl_get_process_apertures_args {
  struct kfd_process_device_apertures process_apertures[NUM_OF_SUPPORTED_GPUS];
  uint32_t num_of_nodes;
  uint32_t pad;
};
#define MAX_ALLOWED_NUM_POINTS 100
#define MAX_ALLOWED_AW_BUFF_SIZE 4096
#define MAX_ALLOWED_WAC_BUFF_SIZE 128
struct kfd_ioctl_dbg_register_args {
  uint32_t gpu_id;
  uint32_t pad;
};
struct kfd_ioctl_dbg_unregister_args {
  uint32_t gpu_id;
  uint32_t pad;
};
struct kfd_ioctl_dbg_address_watch_args {
  uint64_t content_ptr;
  uint32_t gpu_id;
  uint32_t buf_size_in_bytes;
};
struct kfd_ioctl_dbg_wave_control_args {
  uint64_t content_ptr;
  uint32_t gpu_id;
  uint32_t buf_size_in_bytes;
};
#define KFD_IOC_EVENT_SIGNAL 0
#define KFD_IOC_EVENT_NODECHANGE 1
#define KFD_IOC_EVENT_DEVICESTATECHANGE 2
#define KFD_IOC_EVENT_HW_EXCEPTION 3
#define KFD_IOC_EVENT_SYSTEM_EVENT 4
#define KFD_IOC_EVENT_DEBUG_EVENT 5
#define KFD_IOC_EVENT_PROFILE_EVENT 6
#define KFD_IOC_EVENT_QUEUE_EVENT 7
#define KFD_IOC_EVENT_MEMORY 8
#define KFD_IOC_WAIT_RESULT_COMPLETE 0
#define KFD_IOC_WAIT_RESULT_TIMEOUT 1
#define KFD_IOC_WAIT_RESULT_FAIL 2
#define KFD_SIGNAL_EVENT_LIMIT 256
struct kfd_ioctl_create_event_args {
  uint64_t event_page_offset;
  uint32_t event_trigger_data;
  uint32_t event_type;
  uint32_t auto_reset;
  uint32_t node_id;
  uint32_t event_id;
  uint32_t event_slot_index;
};
struct kfd_ioctl_destroy_event_args {
  uint32_t event_id;
  uint32_t pad;
};
struct kfd_ioctl_set_event_args {
  uint32_t event_id;
  uint32_t pad;
};
struct kfd_ioctl_reset_event_args {
  uint32_t event_id;
  uint32_t pad;
};
struct kfd_memory_exception_failure {
  uint32_t NotPresent;
  uint32_t ReadOnly;
  uint32_t NoExecute;
  uint32_t pad;
};
struct kfd_hsa_memory_exception_data {
  struct kfd_memory_exception_failure failure;
  uint64_t va;
  uint32_t gpu_id;
  uint32_t pad;
};
struct kfd_event_data {
  union {
    struct kfd_hsa_memory_exception_data memory_exception_data;
  };
  uint64_t kfd_event_data_ext;
  uint32_t event_id;
  uint32_t pad;
};
struct kfd_ioctl_wait_events_args {
  uint64_t events_ptr;
  uint32_t num_events;
  uint32_t wait_for_all;
  uint32_t timeout;
  uint32_t wait_result;
};
#define AMDKFD_IOCTL_BASE 'K'
#define AMDKFD_IO(nr) _IO(AMDKFD_IOCTL_BASE, nr)
#define AMDKFD_IOR(nr,type) _IOR(AMDKFD_IOCTL_BASE, nr, type)
#define AMDKFD_IOW(nr,type) _IOW(AMDKFD_IOCTL_BASE, nr, type)
#define AMDKFD_IOWR(nr,type) _IOWR(AMDKFD_IOCTL_BASE, nr, type)
#define AMDKFD_IOC_GET_VERSION AMDKFD_IOR(0x01, struct kfd_ioctl_get_version_args)
#define AMDKFD_IOC_CREATE_QUEUE AMDKFD_IOWR(0x02, struct kfd_ioctl_create_queue_args)
#define AMDKFD_IOC_DESTROY_QUEUE AMDKFD_IOWR(0x03, struct kfd_ioctl_destroy_queue_args)
#define AMDKFD_IOC_SET_MEMORY_POLICY AMDKFD_IOW(0x04, struct kfd_ioctl_set_memory_policy_args)
#define AMDKFD_IOC_GET_CLOCK_COUNTERS AMDKFD_IOWR(0x05, struct kfd_ioctl_get_clock_counters_args)
#define AMDKFD_IOC_GET_PROCESS_APERTURES AMDKFD_IOR(0x06, struct kfd_ioctl_get_process_apertures_args)
#define AMDKFD_IOC_UPDATE_QUEUE AMDKFD_IOW(0x07, struct kfd_ioctl_update_queue_args)
#define AMDKFD_IOC_CREATE_EVENT AMDKFD_IOWR(0x08, struct kfd_ioctl_create_event_args)
#define AMDKFD_IOC_DESTROY_EVENT AMDKFD_IOW(0x09, struct kfd_ioctl_destroy_event_args)
#define AMDKFD_IOC_SET_EVENT AMDKFD_IOW(0x0A, struct kfd_ioctl_set_event_args)
#define AMDKFD_IOC_RESET_EVENT AMDKFD_IOW(0x0B, struct kfd_ioctl_reset_event_args)
#define AMDKFD_IOC_WAIT_EVENTS AMDKFD_IOWR(0x0C, struct kfd_ioctl_wait_events_args)
#define AMDKFD_IOC_DBG_REGISTER AMDKFD_IOW(0x0D, struct kfd_ioctl_dbg_register_args)
#define AMDKFD_IOC_DBG_UNREGISTER AMDKFD_IOW(0x0E, struct kfd_ioctl_dbg_unregister_args)
#define AMDKFD_IOC_DBG_ADDRESS_WATCH AMDKFD_IOW(0x0F, struct kfd_ioctl_dbg_address_watch_args)
#define AMDKFD_IOC_DBG_WAVE_CONTROL AMDKFD_IOW(0x10, struct kfd_ioctl_dbg_wave_control_args)
#define AMDKFD_COMMAND_START 0x01
#define AMDKFD_COMMAND_END 0x11
#endif
