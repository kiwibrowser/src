// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_classic_win.h"

namespace {
static device::win::BluetoothClassicWrapper* g_instance_ = nullptr;
}  // namespace

namespace device {
namespace win {

BluetoothClassicWrapper* BluetoothClassicWrapper::GetInstance() {
  if (g_instance_ == nullptr) {
    g_instance_ = new BluetoothClassicWrapper();
  }
  return g_instance_;
}

void BluetoothClassicWrapper::DeleteInstance() {
  delete g_instance_;
  g_instance_ = nullptr;
}

void BluetoothClassicWrapper::SetInstanceForTest(
    BluetoothClassicWrapper* instance) {
  delete g_instance_;
  g_instance_ = instance;
}

BluetoothClassicWrapper::BluetoothClassicWrapper() {}
BluetoothClassicWrapper::~BluetoothClassicWrapper() {}

HBLUETOOTH_RADIO_FIND BluetoothClassicWrapper::FindFirstRadio(
    const BLUETOOTH_FIND_RADIO_PARAMS* params,
    HANDLE* out_handle) {
  HANDLE radio_handle = INVALID_HANDLE_VALUE;
  HBLUETOOTH_RADIO_FIND radio_find_handle =
      BluetoothFindFirstRadio(params, &radio_handle);
  if (radio_find_handle) {
    // TODO(crbug.com/820864): At some point, we crash when we attempt to close
    // the handle.  This and the related checks in this file are designed to
    // see if our handle becomes garbage (i.e., "valid", but not closeable) at
    // some point.
    DWORD info;
    CHECK(!base::win::HandleTraits::IsHandleValid(radio_handle) ||
          ::GetHandleInformation(radio_handle, &info));
    opened_radio_handle_.Set(radio_handle);
    *out_handle = opened_radio_handle_.Get();
  }
  return radio_find_handle;
}

DWORD BluetoothClassicWrapper::GetRadioInfo(
    HANDLE handle,
    PBLUETOOTH_RADIO_INFO out_radio_info) {
  DWORD ret = BluetoothGetRadioInfo(handle, out_radio_info);
  DWORD info;
  // TODO(crbug.com/820864): Remove this check.
  CHECK(::GetHandleInformation(handle, &info));
  return ret;
}

BOOL BluetoothClassicWrapper::FindRadioClose(HBLUETOOTH_RADIO_FIND handle) {
  return BluetoothFindRadioClose(handle);
}

BOOL BluetoothClassicWrapper::IsConnectable(HANDLE handle) {
  DWORD ret = BluetoothIsConnectable(handle);
  DWORD info;
  // TODO(crbug.com/820864): Remove this check.
  CHECK(::GetHandleInformation(handle, &info));
  return ret;
}

HBLUETOOTH_DEVICE_FIND BluetoothClassicWrapper::FindFirstDevice(
    const BLUETOOTH_DEVICE_SEARCH_PARAMS* params,
    BLUETOOTH_DEVICE_INFO* out_device_info) {
  return BluetoothFindFirstDevice(params, out_device_info);
}

BOOL BluetoothClassicWrapper::FindNextDevice(
    HBLUETOOTH_DEVICE_FIND handle,
    BLUETOOTH_DEVICE_INFO* out_device_info) {
  return BluetoothFindNextDevice(handle, out_device_info);
}

BOOL BluetoothClassicWrapper::FindDeviceClose(HBLUETOOTH_DEVICE_FIND handle) {
  return BluetoothFindDeviceClose(handle);
}

BOOL BluetoothClassicWrapper::EnableDiscovery(HANDLE handle, BOOL is_enable) {
  DWORD ret = BluetoothEnableDiscovery(handle, is_enable);
  DWORD info;
  // TODO(crbug.com/820864): Remove this check.
  CHECK(::GetHandleInformation(handle, &info));
  return ret;
}

BOOL BluetoothClassicWrapper::EnableIncomingConnections(HANDLE handle,
                                                        BOOL is_enable) {
  DWORD ret = BluetoothEnableIncomingConnections(handle, is_enable);
  DWORD info;
  // TODO(crbug.com/820864): Remove this check.
  CHECK(::GetHandleInformation(handle, &info));
  return ret;
}

DWORD BluetoothClassicWrapper::LastError() {
  return GetLastError();
}

}  // namespace win
}  // namespace device
