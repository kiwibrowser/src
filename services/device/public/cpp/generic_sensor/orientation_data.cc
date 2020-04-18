// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/public/cpp/generic_sensor/orientation_data.h"

#include <string.h>

namespace device {

OrientationData::OrientationData() {
  // Make sure to zero out the memory so that there are no uninitialized bits.
  // This object is used in the shared memory buffer and is memory copied by
  // two processes. Valgrind will complain if we copy around memory that is
  // only partially initialized.
  memset(this, 0, sizeof(*this));
}

}  // namespace device
