/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/mmap_test_check.h"

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdio.h>

#include "native_client/src/include/nacl_assert.h"


namespace {

// Returns information about one or more VM region located at or above
// |address| by calling |mach_vm_region| repeatedly to determine the size of a
// contiguous region composed of multiple smaller regions that have the same
// protection and sharing flags set. Returns the number of smaller regions
// that were coalesced, which may be 0, in which case the out parameters are
// not valid. |region_address|, |region_size|, |region_protection|, and
// |region_share_mode| are set according to the characteristics of the large
// region.
//
// This is necessary because a single anonymous mmap() for a large region will
// be broken up into 128MB chunks in the kernel's VM map when allocated - see
// ANON_CHUNK_SIZE in 10.8.2 xnu-2050.18.24/osfmk/vm/vm_map.c vm_map_enter.
size_t CoalescedVMRegionInfo(mach_vm_address_t address,
                             mach_vm_address_t *region_address,
                             mach_vm_size_t *region_size,
                             vm_prot_t *region_protection,
                             unsigned char *region_share_mode) {
  mach_port_t self_task = mach_task_self();

  mach_vm_address_t this_region_address = address;
  mach_vm_size_t this_region_size;
  vm_region_extended_info_data_t info;
  mach_msg_type_number_t info_count = VM_REGION_EXTENDED_INFO_COUNT;
  mach_port_t object;
  kern_return_t kr = mach_vm_region(self_task,
                                    &this_region_address,
                                    &this_region_size,
                                    VM_REGION_EXTENDED_INFO,
                                    reinterpret_cast<vm_region_info_t>(&info),
                                    &info_count,
                                    &object);
  if (kr != KERN_SUCCESS) {
    return 0;
  }

  kr = mach_port_deallocate(self_task, object);
  ASSERT_EQ(kr, KERN_SUCCESS);

  size_t regions = 1;
  *region_address = this_region_address;
  *region_size = this_region_size;
  *region_protection = info.protection;
  *region_share_mode = info.share_mode;

  while (true) {
    this_region_address += this_region_size;
    mach_vm_address_t probe_address = this_region_address;

    kr = mach_vm_region(self_task,
                        &this_region_address,
                        &this_region_size,
                        VM_REGION_EXTENDED_INFO,
                        reinterpret_cast<vm_region_info_t>(&info),
                        &info_count,
                        &object);
    if (kr != KERN_SUCCESS) {
      break;
    }

    kr = mach_port_deallocate(self_task, object);
    ASSERT_EQ(kr, KERN_SUCCESS);

    if (this_region_address != probe_address ||
        *region_protection != info.protection ||
        *region_share_mode != info.share_mode) {
      break;
    }

    *region_size += this_region_size;
    ++regions;
  }

  return regions;
}

}  // namespace

void CheckMapping(uintptr_t addr, size_t size, int protect, int map_type) {
  uintptr_t end = addr + size - 1;

  mach_vm_address_t r_start;
  mach_vm_size_t r_size;
  vm_prot_t r_protection;
  unsigned char r_share_mode;
  size_t regions = CoalescedVMRegionInfo(addr,
                                         &r_start,
                                         &r_size,
                                         &r_protection,
                                         &r_share_mode);
  ASSERT_GT(regions, 0);

  mach_vm_address_t r_end = r_start + r_size - 1;

  ASSERT_LE(r_start, addr);
  ASSERT_GE(r_end, end);
  ASSERT_EQ(r_protection, protect);
  // TODO(phosek): not sure whether this check is correct
  ASSERT_EQ(r_share_mode, map_type);
}
