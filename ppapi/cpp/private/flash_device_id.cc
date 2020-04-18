// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/private/flash_device_id.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash_device_id.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_Flash_DeviceID_1_0>() {
  return PPB_FLASH_DEVICEID_INTERFACE_1_0;
}

}  // namespace

namespace flash {

DeviceID::DeviceID() {
}

DeviceID::DeviceID(const InstanceHandle& instance) : Resource() {
  if (has_interface<PPB_Flash_DeviceID_1_0>()) {
    PassRefFromConstructor(get_interface<PPB_Flash_DeviceID_1_0>()->Create(
        instance.pp_instance()));
  }
}

int32_t DeviceID::GetDeviceID(
    const CompletionCallbackWithOutput<Var>& callback) {
  if (has_interface<PPB_Flash_DeviceID_1_0>()) {
    return get_interface<PPB_Flash_DeviceID_1_0>()->GetDeviceID(
        pp_resource(),
        callback.output(),
        callback.pp_completion_callback());
  }
  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

}  // namespace flash
}  // namespace pp
