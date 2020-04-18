// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/connection_medium.h"

#include "base/logging.h"

namespace chromeos {

namespace secure_channel {

std::ostream& operator<<(std::ostream& stream, const ConnectionMedium& medium) {
  DCHECK(medium == ConnectionMedium::kBluetoothLowEnergy);
  stream << "[BLE]";
  return stream;
}

}  // namespace secure_channel

}  // namespace chromeos
