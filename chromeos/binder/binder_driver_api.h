// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include this header for types and constants used by the kernel binder driver.

#ifndef CHROMEOS_BINDER_BINDER_DRIVER_API_H_
#define CHROMEOS_BINDER_BINDER_DRIVER_API_H_

// binder.h needs __packed to be defined.
#define __packed __attribute__((__packed__))

#include <asm/types.h>
#include "chromeos/third_party/android_bionic_libc/kernel/uapi/linux/binder.h"

#endif  // CHROMEOS_BINDER_BINDER_DRIVER_API_H_
