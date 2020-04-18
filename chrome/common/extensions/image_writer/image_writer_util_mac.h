// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_IMAGE_WRITER_IMAGE_WRITER_UTIL_MAC_H_
#define CHROME_COMMON_EXTENSIONS_IMAGE_WRITER_IMAGE_WRITER_UTIL_MAC_H_

#include <IOKit/IOKitLib.h>

namespace extensions {

// Determines whether the object from the IO registry is a USB mass storage
// device.  It does this by reading the ancestors of the object to see if any
// is of the USB mass-storage class.
bool IsUsbDevice(io_object_t disk_obj);

}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_IMAGE_WRITER_IMAGE_WRITER_UTIL_MAC_H_
