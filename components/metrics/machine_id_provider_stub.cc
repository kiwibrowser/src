// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/machine_id_provider.h"

#include "base/logging.h"

namespace metrics {

// static
bool MachineIdProvider::HasId() {
  return false;
}

// static
std::string MachineIdProvider::GetMachineId() {
  NOTREACHED();
  return std::string();
}

}  //  namespace metrics
