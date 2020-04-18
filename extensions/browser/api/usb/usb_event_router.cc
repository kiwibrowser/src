// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/usb/usb_event_router.h"

#include <memory>
#include <utility>

#include "base/lazy_instance.h"
#include "device/base/device_client.h"
#include "device/usb/usb_device.h"
#include "extensions/browser/api/device_permissions_manager.h"
#include "extensions/browser/api/usb/usb_guid_map.h"
#include "extensions/browser/event_router_factory.h"
#include "extensions/common/api/usb.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/permissions/usb_device_permission.h"

namespace usb = extensions::api::usb;

using content::BrowserThread;
using device::UsbDevice;
using device::UsbService;

namespace extensions {

namespace {

// Returns true iff the given extension has permission to receive events
// regarding this device.
bool WillDispatchDeviceEvent(scoped_refptr<UsbDevice> device,
                             content::BrowserContext* browser_context,
                             const Extension* extension,
                             Event* event,
                             const base::DictionaryValue* listener_filter) {
  // Check install-time and optional permissions.
  std::unique_ptr<UsbDevicePermission::CheckParam> param =
      UsbDevicePermission::CheckParam::ForUsbDevice(extension, device.get());
  if (extension->permissions_data()->CheckAPIPermissionWithParam(
          APIPermission::kUsbDevice, param.get())) {
    return true;
  }

  // Check permissions granted through chrome.usb.getUserSelectedDevices.
  DevicePermissions* device_permissions =
      DevicePermissionsManager::Get(browser_context)
          ->GetForExtension(extension->id());
  if (device_permissions->FindUsbDeviceEntry(device).get()) {
    return true;
  }

  return false;
}

base::LazyInstance<BrowserContextKeyedAPIFactory<UsbEventRouter>>::Leaky
    g_event_router_factory = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
BrowserContextKeyedAPIFactory<UsbEventRouter>*
UsbEventRouter::GetFactoryInstance() {
  return g_event_router_factory.Pointer();
}

UsbEventRouter::UsbEventRouter(content::BrowserContext* browser_context)
    : browser_context_(browser_context), observer_(this) {
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (event_router) {
    event_router->RegisterObserver(this, usb::OnDeviceAdded::kEventName);
    event_router->RegisterObserver(this, usb::OnDeviceRemoved::kEventName);
  }
}

UsbEventRouter::~UsbEventRouter() {
}

void UsbEventRouter::Shutdown() {
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (event_router) {
    event_router->UnregisterObserver(this);
  }
}

void UsbEventRouter::OnListenerAdded(const EventListenerInfo& details) {
  UsbService* service = device::DeviceClient::Get()->GetUsbService();
  if (!observer_.IsObserving(service)) {
    observer_.Add(service);
  }
}

void UsbEventRouter::OnDeviceAdded(scoped_refptr<device::UsbDevice> device) {
  DispatchEvent(usb::OnDeviceAdded::kEventName, device);
}

void UsbEventRouter::OnDeviceRemoved(scoped_refptr<device::UsbDevice> device) {
  DispatchEvent(usb::OnDeviceRemoved::kEventName, device);
}

void UsbEventRouter::DispatchEvent(const std::string& event_name,
                                   scoped_refptr<UsbDevice> device) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (event_router) {
    usb::Device device_obj;
    UsbGuidMap::Get(browser_context_)->GetApiDevice(device, &device_obj);

    std::unique_ptr<Event> event;
    if (event_name == usb::OnDeviceAdded::kEventName) {
      event.reset(new Event(events::USB_ON_DEVICE_ADDED,
                            usb::OnDeviceAdded::kEventName,
                            usb::OnDeviceAdded::Create(device_obj)));
    } else {
      DCHECK(event_name == usb::OnDeviceRemoved::kEventName);
      event.reset(new Event(events::USB_ON_DEVICE_REMOVED,
                            usb::OnDeviceRemoved::kEventName,
                            usb::OnDeviceRemoved::Create(device_obj)));
    }

    event->will_dispatch_callback =
        base::Bind(&WillDispatchDeviceEvent, device);
    event_router->BroadcastEvent(std::move(event));
  }
}

template <>
void BrowserContextKeyedAPIFactory<
    UsbEventRouter>::DeclareFactoryDependencies() {
  DependsOn(DevicePermissionsManagerFactory::GetInstance());
  DependsOn(EventRouterFactory::GetInstance());
  DependsOn(UsbGuidMap::GetFactoryInstance());
}

}  // extensions
