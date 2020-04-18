// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SYSTEM_DEVICEMODE_H_
#define CHROMEOS_SYSTEM_DEVICEMODE_H_

#include "chromeos/chromeos_export.h"

namespace chromeos {

// Returns true when running as system compositor. Ie. using libudev, kms and
// evdev for input and output.
CHROMEOS_EXPORT bool IsRunningAsSystemCompositor();

}  // namespace chromeos

#endif  // CHROMEOS_SYSTEM_DEVICEMODE_H_
