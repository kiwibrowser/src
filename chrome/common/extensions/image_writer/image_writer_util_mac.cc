// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/image_writer/image_writer_util_mac.h"

#include <CoreFoundation/CoreFoundation.h>

#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_ioobject.h"

namespace extensions {

bool IsUsbDevice(io_object_t disk_obj) {
  io_object_t current_obj = disk_obj;
  io_object_t parent_obj = 0;
  // Keep scoped object outside the loop so the object lives to the next
  // GetParentEntry.
  base::mac::ScopedIOObject<io_object_t> parent_obj_ref(parent_obj);

  while ((IORegistryEntryGetParentEntry(
             current_obj, kIOServicePlane, &parent_obj)) == KERN_SUCCESS) {
    current_obj = parent_obj;
    parent_obj_ref.reset(parent_obj);

    base::ScopedCFTypeRef<CFStringRef> class_name(
        IOObjectCopyClass(current_obj));
    if (!class_name) {
      LOG(ERROR) << "Could not get object class of IO Registry Entry.";
      continue;
    }

    if (CFStringCompare(class_name.get(), CFSTR("IOUSBMassStorageClass"), 0) ==
        kCFCompareEqualTo) {
      return true;
    }
  }

  return false;
}

}  // namespace extensions
