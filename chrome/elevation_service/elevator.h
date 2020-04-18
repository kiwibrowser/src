// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELEVATION_SERVICE_ELEVATOR_H_
#define CHROME_ELEVATION_SERVICE_ELEVATOR_H_

#include <windows.h>

#include <wrl/implements.h>
#include <wrl/module.h>

#include "base/macros.h"
#include "chrome/elevation_service/elevation_service_idl.h"

namespace elevation_service {

class Elevator
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
          IRegisteredCommandElevator> {
 public:
  Elevator() = default;

  IFACEMETHOD(LaunchCommand)
  (const WCHAR* cmd_id, DWORD caller_proc_id, ULONG_PTR* proc_handle);

 private:
  ~Elevator() override;

  DISALLOW_COPY_AND_ASSIGN(Elevator);
};

}  // namespace elevation_service

#endif  // CHROME_ELEVATION_SERVICE_ELEVATOR_H_
