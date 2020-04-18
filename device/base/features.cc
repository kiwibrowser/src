// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/base/features.h"

#include "build/build_config.h"

namespace device {

#if defined(OS_WIN)
const base::Feature kNewUsbBackend{"NewUsbBackend",
                                   base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kNewBLEWinImplementation{"NewBLEWinImplementation",
                                             base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // defined(OS_WIN)

#if defined(OS_LINUX) || defined(OS_CHROMEOS)
// Enables or disables the use of newblue Bluetooth daemon on Chrome OS.
const base::Feature kNewblueDaemon{"Newblue",
                                   base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // defined(OS_LINUX) || defined(OS_CHROMEOS)

}  // namespace device
