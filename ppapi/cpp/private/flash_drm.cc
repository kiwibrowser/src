// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/private/flash_drm.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash_device_id.h"
#include "ppapi/c/private/ppb_flash_drm.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_Flash_DRM_1_0>() {
  return PPB_FLASH_DRM_INTERFACE_1_0;
}

template <> const char* interface_name<PPB_Flash_DRM_1_1>() {
  return PPB_FLASH_DRM_INTERFACE_1_1;
}

template <> const char* interface_name<PPB_Flash_DeviceID_1_0>() {
  return PPB_FLASH_DEVICEID_INTERFACE_1_0;
}

}  // namespace

namespace flash {

DRM::DRM() {
}

DRM::DRM(const InstanceHandle& instance) : Resource() {
  if (has_interface<PPB_Flash_DRM_1_1>()) {
    PassRefFromConstructor(get_interface<PPB_Flash_DRM_1_1>()->Create(
        instance.pp_instance()));
  } else if (has_interface<PPB_Flash_DRM_1_0>()) {
    PassRefFromConstructor(get_interface<PPB_Flash_DRM_1_0>()->Create(
        instance.pp_instance()));
  } else if (has_interface<PPB_Flash_DeviceID_1_0>()) {
    PassRefFromConstructor(get_interface<PPB_Flash_DeviceID_1_0>()->Create(
        instance.pp_instance()));
  }
}

int32_t DRM::GetDeviceID(const CompletionCallbackWithOutput<Var>& callback) {
  if (has_interface<PPB_Flash_DRM_1_1>()) {
    return get_interface<PPB_Flash_DRM_1_1>()->GetDeviceID(
        pp_resource(),
        callback.output(),
        callback.pp_completion_callback());
  }
  if (has_interface<PPB_Flash_DRM_1_0>()) {
    return get_interface<PPB_Flash_DRM_1_0>()->GetDeviceID(
        pp_resource(),
        callback.output(),
        callback.pp_completion_callback());
  }
  if (has_interface<PPB_Flash_DeviceID_1_0>()) {
    return get_interface<PPB_Flash_DeviceID_1_0>()->GetDeviceID(
        pp_resource(),
        callback.output(),
        callback.pp_completion_callback());
  }
  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

bool DRM::GetHmonitor(int64_t* hmonitor) {
  if (has_interface<PPB_Flash_DRM_1_1>()) {
    return PP_ToBool(get_interface<PPB_Flash_DRM_1_1>()->GetHmonitor(
        pp_resource(),
        hmonitor));
  }
  if (has_interface<PPB_Flash_DRM_1_0>()) {
    return PP_ToBool(get_interface<PPB_Flash_DRM_1_0>()->GetHmonitor(
        pp_resource(),
        hmonitor));
  }
  return 0;
}

int32_t DRM::GetVoucherFile(
    const CompletionCallbackWithOutput<FileRef>& callback) {
  if (has_interface<PPB_Flash_DRM_1_1>()) {
    return get_interface<PPB_Flash_DRM_1_1>()->GetVoucherFile(
        pp_resource(),
        callback.output(),
        callback.pp_completion_callback());
  }
  if (has_interface<PPB_Flash_DRM_1_0>()) {
    return get_interface<PPB_Flash_DRM_1_0>()->GetVoucherFile(
        pp_resource(),
        callback.output(),
        callback.pp_completion_callback());
  }
  return PP_ERROR_NOINTERFACE;
}

int32_t DRM::MonitorIsExternal(
    const CompletionCallbackWithOutput<PP_Bool>& callback) {
  if (has_interface<PPB_Flash_DRM_1_1>()) {
    return get_interface<PPB_Flash_DRM_1_1>()->MonitorIsExternal(
        pp_resource(),
        callback.output(),
        callback.pp_completion_callback());
  }
  return PP_ERROR_NOINTERFACE;
}

}  // namespace flash
}  // namespace pp
