// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_MACHINE_ID_PROVIDER_H_
#define COMPONENTS_METRICS_MACHINE_ID_PROVIDER_H_

#include <string>

#include "base/macros.h"

namespace metrics {

// Provides machine characteristics used as a machine id. The implementation is
// platform specific. GetMachineId() must be called on a thread which allows
// I/O. GetMachineId() must not be called if HasId() returns false on this
// platform.
class MachineIdProvider {
 public:
  // Returns true if this platform provides a non-empty GetMachineId(). This is
  // useful to avoid an async call to GetMachineId() on platforms with no
  // implementation.
  static bool HasId();

  // Get a string containing machine characteristics, to be used as a machine
  // id. The implementation is platform specific, with a default implementation
  // returning an empty string.
  // The return value should not be stored to disk or transmitted.
  static std::string GetMachineId();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(MachineIdProvider);
};

}  //  namespace metrics

#endif  // COMPONENTS_METRICS_MACHINE_ID_PROVIDER_H_
