// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/base/device_client.h"

#include "base/logging.h"

namespace device {

namespace {

DeviceClient* g_instance;

}  // namespace

DeviceClient::DeviceClient() {
  g_instance = this;
}

DeviceClient::~DeviceClient() {
  g_instance = nullptr;
}

/* static */
DeviceClient* DeviceClient::Get() {
  DCHECK(g_instance);
  return g_instance;
}

UsbService* DeviceClient::GetUsbService() {
  // This should never be called by clients which do not support the USB API.
  NOTREACHED();
  return nullptr;
}

}  // namespace device
