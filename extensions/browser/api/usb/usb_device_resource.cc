// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/usb/usb_device_resource.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/lazy_instance.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/browser_thread.h"
#include "device/usb/usb_device_handle.h"
#include "extensions/browser/api/api_resource.h"
#include "extensions/common/api/usb.h"

using content::BrowserThread;
using device::UsbDeviceHandle;

namespace extensions {

static base::LazyInstance<BrowserContextKeyedAPIFactory<
    ApiResourceManager<UsbDeviceResource>>>::DestructorAtExit g_factory =
    LAZY_INSTANCE_INITIALIZER;

// static
template <>
BrowserContextKeyedAPIFactory<ApiResourceManager<UsbDeviceResource> >*
ApiResourceManager<UsbDeviceResource>::GetFactoryInstance() {
  return g_factory.Pointer();
}

UsbDeviceResource::UsbDeviceResource(const std::string& owner_extension_id,
                                     scoped_refptr<UsbDeviceHandle> device)
    : ApiResource(owner_extension_id), device_(device) {
}

UsbDeviceResource::~UsbDeviceResource() {
  device_->Close();
}

bool UsbDeviceResource::IsPersistent() const {
  return false;
}

}  // namespace extensions
