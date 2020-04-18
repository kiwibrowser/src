// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/usb/usb_host_bridge.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/permission_broker_client.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_features.h"
#include "components/arc/usb/usb_host_ui_delegate.h"
#include "device/base/device_client.h"
#include "device/usb/mojo/type_converters.h"
#include "device/usb/usb_device_handle.h"
#include "device/usb/usb_device_linux.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace arc {
namespace {

// Singleton factory for ArcUsbHostBridge
class ArcUsbHostBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcUsbHostBridge,
          ArcUsbHostBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcUsbHostBridgeFactory";

  static ArcUsbHostBridgeFactory* GetInstance() {
    return base::Singleton<ArcUsbHostBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcUsbHostBridgeFactory>;
  ArcUsbHostBridgeFactory() = default;
  ~ArcUsbHostBridgeFactory() override = default;
};

void OnDeviceOpened(mojom::UsbHostHost::OpenDeviceCallback callback,
                    base::ScopedFD fd) {
  if (!fd.is_valid()) {
    LOG(ERROR) << "Invalid USB device FD";
    std::move(callback).Run(mojo::ScopedHandle());
    return;
  }
  mojo::edk::ScopedInternalPlatformHandle platform_handle{
      mojo::edk::InternalPlatformHandle(fd.release())};
  MojoHandle wrapped_handle;
  MojoResult wrap_result = mojo::edk::CreateInternalPlatformHandleWrapper(
      std::move(platform_handle), &wrapped_handle);
  if (wrap_result != MOJO_RESULT_OK) {
    LOG(ERROR) << "Failed to wrap device FD. Closing: " << wrap_result;
    std::move(callback).Run(mojo::ScopedHandle());
    return;
  }
  mojo::ScopedHandle scoped_handle{mojo::Handle(wrapped_handle)};
  std::move(callback).Run(std::move(scoped_handle));
}

void OnDeviceOpenError(mojom::UsbHostHost::OpenDeviceCallback callback,
                       const std::string& error_name,
                       const std::string& error_message) {
  LOG(WARNING) << "Cannot open USB device: " << error_name << ": "
               << error_message;
  std::move(callback).Run(mojo::ScopedHandle());
}

using CheckedCallback =
    base::RepeatingCallback<void(const std::string& guid, bool success)>;

void OnGetDevicesComplete(
    const CheckedCallback& callback,
    const std::vector<scoped_refptr<device::UsbDevice>>& devices) {
  for (const scoped_refptr<device::UsbDevice>& device : devices)
    device->CheckUsbAccess(base::BindOnce(callback, device.get()->guid()));
}

}  // namespace

ArcUsbHostBridge* ArcUsbHostBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcUsbHostBridgeFactory::GetForBrowserContext(context);
}

ArcUsbHostBridge::ArcUsbHostBridge(content::BrowserContext* context,
                                   ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      usb_observer_(this),
      weak_factory_(this) {
  arc_bridge_service_->usb_host()->SetHost(this);
  arc_bridge_service_->usb_host()->AddObserver(this);

  usb_service_ = device::DeviceClient::Get()->GetUsbService();
  if (usb_service_)
    usb_observer_.Add(usb_service_);
}

ArcUsbHostBridge::~ArcUsbHostBridge() {
  if (usb_service_)
    usb_service_->RemoveObserver(this);
  arc_bridge_service_->usb_host()->RemoveObserver(this);
  arc_bridge_service_->usb_host()->SetHost(nullptr);
}

BrowserContextKeyedServiceFactory* ArcUsbHostBridge::GetFactory() {
  return ArcUsbHostBridgeFactory::GetInstance();
}

void ArcUsbHostBridge::RequestPermission(const std::string& guid,
                                         const std::string& package,
                                         bool interactive,
                                         RequestPermissionCallback callback) {
  if (guid.empty()) {
    HandleScanDeviceListRequest(package, std::move(callback));
    return;
  }

  VLOG(2) << "USB RequestPermission " << guid << " package " << package;
  // Permission already requested.
  if (HasPermissionForDevice(guid, package)) {
    std::move(callback).Run(true);
    return;
  }

  // The other side was just checking, fail without asking the user.
  if (!interactive) {
    std::move(callback).Run(HasPermissionForDevice(guid, package));
    return;
  }

  // Ask the authorization from the user.
  DoRequestUserAuthorization(guid, package, std::move(callback));
}

void ArcUsbHostBridge::OpenDevice(const std::string& guid,
                                  const base::Optional<std::string>& package,
                                  OpenDeviceCallback callback) {
  if (!usb_service_ || !package) {
    std::move(callback).Run(mojo::ScopedHandle());
    return;
  }

  device::UsbDeviceLinux* device =
      static_cast<device::UsbDeviceLinux*>(usb_service_->GetDevice(guid).get());
  if (!device) {
    std::move(callback).Run(mojo::ScopedHandle());
    return;
  }

  // The RequestPermission was never done, abort.
  if (!HasPermissionForDevice(guid, package.value())) {
    std::move(callback).Run(mojo::ScopedHandle());
    return;
  }

  chromeos::PermissionBrokerClient* client =
      chromeos::DBusThreadManager::Get()->GetPermissionBrokerClient();
  DCHECK(client) << "Could not get permission broker client.";
  auto repeating_callback =
      base::AdaptCallbackForRepeating(std::move(callback));
  client->OpenPath(device->device_path(),
                   base::Bind(&OnDeviceOpened, repeating_callback),
                   base::Bind(&OnDeviceOpenError, repeating_callback));
}

void ArcUsbHostBridge::GetDeviceInfo(const std::string& guid,
                                     GetDeviceInfoCallback callback) {
  if (!usb_service_) {
    std::move(callback).Run(std::string(), nullptr);
    return;
  }
  scoped_refptr<device::UsbDevice> device = usb_service_->GetDevice(guid);
  if (!device.get()) {
    LOG(WARNING) << "Unknown USB device " << guid;
    std::move(callback).Run(std::string(), nullptr);
    return;
  }

  device::mojom::UsbDeviceInfoPtr info =
      device::mojom::UsbDeviceInfo::From(*device);
  // b/69295049 the other side doesn't like optional strings.
  for (const device::mojom::UsbConfigurationInfoPtr& cfg :
       info->configurations) {
    cfg->configuration_name =
        cfg->configuration_name.value_or(base::string16());
    for (const device::mojom::UsbInterfaceInfoPtr& iface : cfg->interfaces) {
      for (const device::mojom::UsbAlternateInterfaceInfoPtr& alt :
           iface->alternates) {
        alt->interface_name = alt->interface_name.value_or(base::string16());
      }
    }
  }

  std::string path =
      static_cast<device::UsbDeviceLinux*>(device.get())->device_path();

  std::move(callback).Run(path, std::move(info));
}

// device::UsbService::Observer callbacks.

void ArcUsbHostBridge::OnDeviceAdded(scoped_refptr<device::UsbDevice> device) {
  device->CheckUsbAccess(base::BindOnce(&ArcUsbHostBridge::OnDeviceChecked,
                                        weak_factory_.GetWeakPtr(),
                                        device.get()->guid()));
}

void ArcUsbHostBridge::OnDeviceRemoved(
    scoped_refptr<device::UsbDevice> device) {
  mojom::UsbHostInstance* usb_host_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->usb_host(), OnDeviceAdded);

  if (!usb_host_instance) {
    VLOG(2) << "UsbInstance not ready yet";
    return;
  }

  usb_host_instance->OnDeviceRemoved(
      device.get()->guid(), GetEventReceiverPackages(device.get()->guid()));

  if (ui_delegate_)
    ui_delegate_->DeviceRemoved(device.get()->guid());
}

// Notifies the observer that the UsbService it depends on is shutting down.
void ArcUsbHostBridge::WillDestroyUsbService() {
  // Disconnect.
  arc_bridge_service_->usb_host()->SetHost(nullptr);
}

void ArcUsbHostBridge::OnConnectionReady() {
  if (!usb_service_)
    return;
  // Send the (filtered) list of already existing USB devices to the other side.
  usb_service_->GetDevices(
      base::Bind(&OnGetDevicesComplete,
                 base::BindRepeating(&ArcUsbHostBridge::OnDeviceChecked,
                                     weak_factory_.GetWeakPtr())));
}

void ArcUsbHostBridge::OnConnectionClosed() {
  if (ui_delegate_)
    ui_delegate_->ClearPermissionRequests();
}

void ArcUsbHostBridge::Shutdown() {
  ui_delegate_ = nullptr;
}

void ArcUsbHostBridge::SetUiDelegate(ArcUsbHostUiDelegate* ui_delegate) {
  ui_delegate_ = ui_delegate;
}

std::vector<std::string> ArcUsbHostBridge::GetEventReceiverPackages(
    const std::string& guid) {
  scoped_refptr<device::UsbDevice> device = usb_service_->GetDevice(guid);
  if (!device.get()) {
    LOG(WARNING) << "Unknown USB device " << guid;
    return std::vector<std::string>();
  }

  if (!ui_delegate_)
    return std::vector<std::string>();

  std::unordered_set<std::string> receivers = ui_delegate_->GetEventPackageList(
      guid, device->serial_number(), device->vendor_id(), device->product_id());

  return std::vector<std::string>(receivers.begin(), receivers.end());
}

void ArcUsbHostBridge::OnDeviceChecked(const std::string& guid, bool allowed) {
  if (!base::FeatureList::IsEnabled(arc::kUsbHostFeature)) {
    VLOG(1) << "AndroidUSBHost: feature is disabled; ignoring";
    return;
  }

  if (!allowed)
    return;

  mojom::UsbHostInstance* usb_host_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->usb_host(), OnDeviceAdded);

  if (!usb_host_instance)
    return;

  usb_host_instance->OnDeviceAdded(guid, GetEventReceiverPackages(guid));
}

void ArcUsbHostBridge::DoRequestUserAuthorization(
    const std::string& guid,
    const std::string& package,
    RequestPermissionCallback callback) {
  if (!ui_delegate_) {
    std::move(callback).Run(false);
    return;
  }

  if (!usb_service_) {
    std::move(callback).Run(false);
    return;
  }

  scoped_refptr<device::UsbDevice> device = usb_service_->GetDevice(guid);
  if (!device.get()) {
    LOG(WARNING) << "Unknown USB device " << guid;
    std::move(callback).Run(false);
    return;
  }

  ui_delegate_->RequestUsbAccessPermission(
      package, guid, device->serial_number(), device->manufacturer_string(),
      device->product_string(), device->vendor_id(), device->product_id(),
      std::move(callback));
}

bool ArcUsbHostBridge::HasPermissionForDevice(const std::string& guid,
                                              const std::string& package) {
  if (!ui_delegate_)
    return false;

  if (!usb_service_)
    return false;

  scoped_refptr<device::UsbDevice> device = usb_service_->GetDevice(guid);
  if (!device.get()) {
    LOG(WARNING) << "Unknown USB device " << guid;
    return false;
  }

  return ui_delegate_->HasUsbAccessPermission(
      package, guid, device->serial_number(), device->vendor_id(),
      device->product_id());
}

void ArcUsbHostBridge::HandleScanDeviceListRequest(
    const std::string& package,
    RequestPermissionCallback callback) {
  if (!ui_delegate_) {
    std::move(callback).Run(false);
    return;
  }

  VLOG(2) << "USB Request USB scan devicelist permission "
          << "package: " << package;
  ui_delegate_->RequestUsbScanDeviceListPermission(package,
                                                   std::move(callback));
}

}  // namespace arc
