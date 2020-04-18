// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_USB_USB_GUID_MAP_H_
#define EXTENSIONS_BROWSER_API_USB_USB_GUID_MAP_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "device/usb/usb_service.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/common/api/usb.h"

namespace device {
class UsbDevice;
}

namespace extensions {

// A BrowserContext-scoped object which maps USB device GUIDs to legacy integer
// IDs for use with the extensions API. This observes device removal to keep
// the mapping from growing indefinitely.
class UsbGuidMap : public BrowserContextKeyedAPI,
                   public device::UsbService::Observer {
 public:
  static UsbGuidMap* Get(content::BrowserContext* browser_context);

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<UsbGuidMap>* GetFactoryInstance();

  // Returns an ID for this device GUID. If the GUID is unknown to the
  // UsbGuidMap a new ID is generated for it.
  int GetIdFromGuid(const std::string& guid);

  // Looks up a device GUID for a given extensions USB device ID. If the ID is
  // unknown (e.g., the corresponding device was unplugged), this returns
  // |false|; otherwise it returns |true|.
  bool GetGuidFromId(int id, std::string* guid);

  // Populates an instance of the chrome.usb.Device object from the given
  // device.
  void GetApiDevice(scoped_refptr<const device::UsbDevice> device_in,
                    extensions::api::usb::Device* device_out);

 private:
  friend class BrowserContextKeyedAPIFactory<UsbGuidMap>;

  explicit UsbGuidMap(content::BrowserContext* context);
  ~UsbGuidMap() override;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "UsbGuidMap"; }
  static const bool kServiceIsCreatedWithBrowserContext = false;
  static const bool kServiceRedirectedInIncognito = true;

  // UsbService::Observer implementation.
  void OnDeviceRemovedCleanup(scoped_refptr<device::UsbDevice> device) override;

  content::BrowserContext* const browser_context_;

  int next_id_ = 0;
  std::map<std::string, int> guid_to_id_map_;
  std::map<int, std::string> id_to_guid_map_;

  ScopedObserver<device::UsbService, device::UsbService::Observer> observer_;

  DISALLOW_COPY_AND_ASSIGN(UsbGuidMap);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_USB_USB_GUID_MAP_H_
