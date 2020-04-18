// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_USB_USB_HOST_BRIDGE_H_
#define COMPONENTS_ARC_USB_USB_HOST_BRIDGE_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "components/arc/common/usb_host.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "device/usb/public/mojom/device_manager.mojom.h"
#include "device/usb/usb_device.h"
#include "device/usb/usb_service.h"

namespace content {
class BrowserContext;
}  // namespace content

class BrowserContextKeyedServiceFactory;

namespace arc {

class ArcBridgeService;
class ArcUsbHostUiDelegate;

// Private implementation of UsbHostHost.
class ArcUsbHostBridge : public KeyedService,
                         public ConnectionObserver<mojom::UsbHostInstance>,
                         public device::UsbService::Observer,
                         public mojom::UsbHostHost {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcUsbHostBridge* GetForBrowserContext(
      content::BrowserContext* context);

  // The constructor will register an Observer with ArcBridgeService.
  explicit ArcUsbHostBridge(content::BrowserContext* context,
                            ArcBridgeService* bridge_service);
  ~ArcUsbHostBridge() override;

  // Returns the factory instance for this class.
  static BrowserContextKeyedServiceFactory* GetFactory();

  // mojom::UsbHostHost overrides:
  void RequestPermission(const std::string& guid,
                         const std::string& package,
                         bool interactive,
                         RequestPermissionCallback callback) override;
  void OpenDevice(const std::string& guid,
                  const base::Optional<std::string>& package,
                  OpenDeviceCallback callback) override;
  void GetDeviceInfo(const std::string& guid,
                     GetDeviceInfoCallback callback) override;

  // device::UsbService::Observer overrides:
  void OnDeviceAdded(scoped_refptr<device::UsbDevice> device) override;
  void OnDeviceRemoved(scoped_refptr<device::UsbDevice> device) override;
  void WillDestroyUsbService() override;

  // ConnectionObserver<mojom::UsbHostInstance> overrides:
  void OnConnectionReady() override;
  void OnConnectionClosed() override;

  // KeyedService overrides:
  void Shutdown() override;

  void SetUiDelegate(ArcUsbHostUiDelegate* ui_delegate);

 private:
  std::vector<std::string> GetEventReceiverPackages(const std::string& guid);
  void OnDeviceChecked(const std::string& guid, bool allowed);
  void DoRequestUserAuthorization(const std::string& guid,
                                  const std::string& package,
                                  RequestPermissionCallback callback);
  bool HasPermissionForDevice(const std::string& guid,
                              const std::string& package);
  void HandleScanDeviceListRequest(const std::string& package,
                                   RequestPermissionCallback callback);

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  mojom::UsbHostHostPtr usb_host_ptr_;
  ScopedObserver<device::UsbService, device::UsbService::Observer>
      usb_observer_;
  device::UsbService* usb_service_;
  ArcUsbHostUiDelegate* ui_delegate_ = nullptr;

  // WeakPtrFactory to use for callbacks.
  base::WeakPtrFactory<ArcUsbHostBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcUsbHostBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_USB_USB_HOST_BRIDGE_H_
