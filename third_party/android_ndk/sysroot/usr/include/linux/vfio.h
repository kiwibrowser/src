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
#ifndef _UAPIVFIO_H
#define _UAPIVFIO_H
#include <linux/types.h>
#include <linux/ioctl.h>
#define VFIO_API_VERSION 0
#define VFIO_TYPE1_IOMMU 1
#define VFIO_SPAPR_TCE_IOMMU 2
#define VFIO_TYPE1v2_IOMMU 3
#define VFIO_DMA_CC_IOMMU 4
#define VFIO_EEH 5
#define VFIO_TYPE1_NESTING_IOMMU 6
#define VFIO_SPAPR_TCE_v2_IOMMU 7
#define VFIO_NOIOMMU_IOMMU 8
#define VFIO_TYPE (';')
#define VFIO_BASE 100
struct vfio_info_cap_header {
  __u16 id;
  __u16 version;
  __u32 next;
};
#define VFIO_GET_API_VERSION _IO(VFIO_TYPE, VFIO_BASE + 0)
#define VFIO_CHECK_EXTENSION _IO(VFIO_TYPE, VFIO_BASE + 1)
#define VFIO_SET_IOMMU _IO(VFIO_TYPE, VFIO_BASE + 2)
struct vfio_group_status {
  __u32 argsz;
  __u32 flags;
#define VFIO_GROUP_FLAGS_VIABLE (1 << 0)
#define VFIO_GROUP_FLAGS_CONTAINER_SET (1 << 1)
};
#define VFIO_GROUP_GET_STATUS _IO(VFIO_TYPE, VFIO_BASE + 3)
#define VFIO_GROUP_SET_CONTAINER _IO(VFIO_TYPE, VFIO_BASE + 4)
#define VFIO_GROUP_UNSET_CONTAINER _IO(VFIO_TYPE, VFIO_BASE + 5)
#define VFIO_GROUP_GET_DEVICE_FD _IO(VFIO_TYPE, VFIO_BASE + 6)
struct vfio_device_info {
  __u32 argsz;
  __u32 flags;
#define VFIO_DEVICE_FLAGS_RESET (1 << 0)
#define VFIO_DEVICE_FLAGS_PCI (1 << 1)
#define VFIO_DEVICE_FLAGS_PLATFORM (1 << 2)
#define VFIO_DEVICE_FLAGS_AMBA (1 << 3)
#define VFIO_DEVICE_FLAGS_CCW (1 << 4)
  __u32 num_regions;
  __u32 num_irqs;
};
#define VFIO_DEVICE_GET_INFO _IO(VFIO_TYPE, VFIO_BASE + 7)
#define VFIO_DEVICE_API_PCI_STRING "vfio-pci"
#define VFIO_DEVICE_API_PLATFORM_STRING "vfio-platform"
#define VFIO_DEVICE_API_AMBA_STRING "vfio-amba"
#define VFIO_DEVICE_API_CCW_STRING "vfio-ccw"
struct vfio_region_info {
  __u32 argsz;
  __u32 flags;
#define VFIO_REGION_INFO_FLAG_READ (1 << 0)
#define VFIO_REGION_INFO_FLAG_WRITE (1 << 1)
#define VFIO_REGION_INFO_FLAG_MMAP (1 << 2)
#define VFIO_REGION_INFO_FLAG_CAPS (1 << 3)
  __u32 index;
  __u32 cap_offset;
  __u64 size;
  __u64 offset;
};
#define VFIO_DEVICE_GET_REGION_INFO _IO(VFIO_TYPE, VFIO_BASE + 8)
#define VFIO_REGION_INFO_CAP_SPARSE_MMAP 1
struct vfio_region_sparse_mmap_area {
  __u64 offset;
  __u64 size;
};
struct vfio_region_info_cap_sparse_mmap {
  struct vfio_info_cap_header header;
  __u32 nr_areas;
  __u32 reserved;
  struct vfio_region_sparse_mmap_area areas[];
};
#define VFIO_REGION_INFO_CAP_TYPE 2
struct vfio_region_info_cap_type {
  struct vfio_info_cap_header header;
  __u32 type;
  __u32 subtype;
};
#define VFIO_REGION_TYPE_PCI_VENDOR_TYPE (1 << 31)
#define VFIO_REGION_TYPE_PCI_VENDOR_MASK (0xffff)
#define VFIO_REGION_SUBTYPE_INTEL_IGD_OPREGION (1)
#define VFIO_REGION_SUBTYPE_INTEL_IGD_HOST_CFG (2)
#define VFIO_REGION_SUBTYPE_INTEL_IGD_LPC_CFG (3)
struct vfio_irq_info {
  __u32 argsz;
  __u32 flags;
#define VFIO_IRQ_INFO_EVENTFD (1 << 0)
#define VFIO_IRQ_INFO_MASKABLE (1 << 1)
#define VFIO_IRQ_INFO_AUTOMASKED (1 << 2)
#define VFIO_IRQ_INFO_NORESIZE (1 << 3)
  __u32 index;
  __u32 count;
};
#define VFIO_DEVICE_GET_IRQ_INFO _IO(VFIO_TYPE, VFIO_BASE + 9)
struct vfio_irq_set {
  __u32 argsz;
  __u32 flags;
#define VFIO_IRQ_SET_DATA_NONE (1 << 0)
#define VFIO_IRQ_SET_DATA_BOOL (1 << 1)
#define VFIO_IRQ_SET_DATA_EVENTFD (1 << 2)
#define VFIO_IRQ_SET_ACTION_MASK (1 << 3)
#define VFIO_IRQ_SET_ACTION_UNMASK (1 << 4)
#define VFIO_IRQ_SET_ACTION_TRIGGER (1 << 5)
  __u32 index;
  __u32 start;
  __u32 count;
  __u8 data[];
};
#define VFIO_DEVICE_SET_IRQS _IO(VFIO_TYPE, VFIO_BASE + 10)
#define VFIO_IRQ_SET_DATA_TYPE_MASK (VFIO_IRQ_SET_DATA_NONE | VFIO_IRQ_SET_DATA_BOOL | VFIO_IRQ_SET_DATA_EVENTFD)
#define VFIO_IRQ_SET_ACTION_TYPE_MASK (VFIO_IRQ_SET_ACTION_MASK | VFIO_IRQ_SET_ACTION_UNMASK | VFIO_IRQ_SET_ACTION_TRIGGER)
#define VFIO_DEVICE_RESET _IO(VFIO_TYPE, VFIO_BASE + 11)
enum {
  VFIO_PCI_BAR0_REGION_INDEX,
  VFIO_PCI_BAR1_REGION_INDEX,
  VFIO_PCI_BAR2_REGION_INDEX,
  VFIO_PCI_BAR3_REGION_INDEX,
  VFIO_PCI_BAR4_REGION_INDEX,
  VFIO_PCI_BAR5_REGION_INDEX,
  VFIO_PCI_ROM_REGION_INDEX,
  VFIO_PCI_CONFIG_REGION_INDEX,
  VFIO_PCI_VGA_REGION_INDEX,
  VFIO_PCI_NUM_REGIONS = 9
};
enum {
  VFIO_PCI_INTX_IRQ_INDEX,
  VFIO_PCI_MSI_IRQ_INDEX,
  VFIO_PCI_MSIX_IRQ_INDEX,
  VFIO_PCI_ERR_IRQ_INDEX,
  VFIO_PCI_REQ_IRQ_INDEX,
  VFIO_PCI_NUM_IRQS
};
enum {
  VFIO_CCW_CONFIG_REGION_INDEX,
  VFIO_CCW_NUM_REGIONS
};
enum {
  VFIO_CCW_IO_IRQ_INDEX,
  VFIO_CCW_NUM_IRQS
};
struct vfio_pci_dependent_device {
  __u32 group_id;
  __u16 segment;
  __u8 bus;
  __u8 devfn;
};
struct vfio_pci_hot_reset_info {
  __u32 argsz;
  __u32 flags;
  __u32 count;
  struct vfio_pci_dependent_device devices[];
};
#define VFIO_DEVICE_GET_PCI_HOT_RESET_INFO _IO(VFIO_TYPE, VFIO_BASE + 12)
struct vfio_pci_hot_reset {
  __u32 argsz;
  __u32 flags;
  __u32 count;
  __s32 group_fds[];
};
#define VFIO_DEVICE_PCI_HOT_RESET _IO(VFIO_TYPE, VFIO_BASE + 13)
struct vfio_iommu_type1_info {
  __u32 argsz;
  __u32 flags;
#define VFIO_IOMMU_INFO_PGSIZES (1 << 0)
  __u64 iova_pgsizes;
};
#define VFIO_IOMMU_GET_INFO _IO(VFIO_TYPE, VFIO_BASE + 12)
struct vfio_iommu_type1_dma_map {
  __u32 argsz;
  __u32 flags;
#define VFIO_DMA_MAP_FLAG_READ (1 << 0)
#define VFIO_DMA_MAP_FLAG_WRITE (1 << 1)
  __u64 vaddr;
  __u64 iova;
  __u64 size;
};
#define VFIO_IOMMU_MAP_DMA _IO(VFIO_TYPE, VFIO_BASE + 13)
struct vfio_iommu_type1_dma_unmap {
  __u32 argsz;
  __u32 flags;
  __u64 iova;
  __u64 size;
};
#define VFIO_IOMMU_UNMAP_DMA _IO(VFIO_TYPE, VFIO_BASE + 14)
#define VFIO_IOMMU_ENABLE _IO(VFIO_TYPE, VFIO_BASE + 15)
#define VFIO_IOMMU_DISABLE _IO(VFIO_TYPE, VFIO_BASE + 16)
struct vfio_iommu_spapr_tce_ddw_info {
  __u64 pgsizes;
  __u32 max_dynamic_windows_supported;
  __u32 levels;
};
struct vfio_iommu_spapr_tce_info {
  __u32 argsz;
  __u32 flags;
#define VFIO_IOMMU_SPAPR_INFO_DDW (1 << 0)
  __u32 dma32_window_start;
  __u32 dma32_window_size;
  struct vfio_iommu_spapr_tce_ddw_info ddw;
};
#define VFIO_IOMMU_SPAPR_TCE_GET_INFO _IO(VFIO_TYPE, VFIO_BASE + 12)
struct vfio_eeh_pe_err {
  __u32 type;
  __u32 func;
  __u64 addr;
  __u64 mask;
};
struct vfio_eeh_pe_op {
  __u32 argsz;
  __u32 flags;
  __u32 op;
  union {
    struct vfio_eeh_pe_err err;
  };
};
#define VFIO_EEH_PE_DISABLE 0
#define VFIO_EEH_PE_ENABLE 1
#define VFIO_EEH_PE_UNFREEZE_IO 2
#define VFIO_EEH_PE_UNFREEZE_DMA 3
#define VFIO_EEH_PE_GET_STATE 4
#define VFIO_EEH_PE_STATE_NORMAL 0
#define VFIO_EEH_PE_STATE_RESET 1
#define VFIO_EEH_PE_STATE_STOPPED 2
#define VFIO_EEH_PE_STATE_STOPPED_DMA 4
#define VFIO_EEH_PE_STATE_UNAVAIL 5
#define VFIO_EEH_PE_RESET_DEACTIVATE 5
#define VFIO_EEH_PE_RESET_HOT 6
#define VFIO_EEH_PE_RESET_FUNDAMENTAL 7
#define VFIO_EEH_PE_CONFIGURE 8
#define VFIO_EEH_PE_INJECT_ERR 9
#define VFIO_EEH_PE_OP _IO(VFIO_TYPE, VFIO_BASE + 21)
struct vfio_iommu_spapr_register_memory {
  __u32 argsz;
  __u32 flags;
  __u64 vaddr;
  __u64 size;
};
#define VFIO_IOMMU_SPAPR_REGISTER_MEMORY _IO(VFIO_TYPE, VFIO_BASE + 17)
#define VFIO_IOMMU_SPAPR_UNREGISTER_MEMORY _IO(VFIO_TYPE, VFIO_BASE + 18)
struct vfio_iommu_spapr_tce_create {
  __u32 argsz;
  __u32 flags;
  __u32 page_shift;
  __u32 __resv1;
  __u64 window_size;
  __u32 levels;
  __u32 __resv2;
  __u64 start_addr;
};
#define VFIO_IOMMU_SPAPR_TCE_CREATE _IO(VFIO_TYPE, VFIO_BASE + 19)
struct vfio_iommu_spapr_tce_remove {
  __u32 argsz;
  __u32 flags;
  __u64 start_addr;
};
#define VFIO_IOMMU_SPAPR_TCE_REMOVE _IO(VFIO_TYPE, VFIO_BASE + 20)
#endif
