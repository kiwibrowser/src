// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_HID_HID_MANAGER_IMPL_H_
#define SERVICES_DEVICE_HID_HID_MANAGER_IMPL_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/device/hid/hid_device_info.h"
#include "services/device/hid/hid_service.h"
#include "services/device/public/mojom/hid.mojom.h"

namespace device {

// HidManagerImpl is owned by Device Service. It is reponsible for handling mojo
// communications from clients. It delegates to HidService the real work of
// talking with different platforms.
class HidManagerImpl : public mojom::HidManager, public HidService::Observer {
 public:
  HidManagerImpl();
  ~HidManagerImpl() override;

  // SetHidServiceForTesting only effects the next call to HidManagerImpl's
  // constructor in which the HidManagerImpl will take over the ownership of
  // passed |hid_service|.
  static void SetHidServiceForTesting(std::unique_ptr<HidService> hid_service);

  void AddBinding(mojom::HidManagerRequest request);

  // mojom::HidManager implementation:
  void GetDevicesAndSetClient(mojom::HidManagerClientAssociatedPtrInfo client,
                              GetDevicesCallback callback) override;
  void GetDevices(GetDevicesCallback callback) override;
  void Connect(const std::string& device_guid,
               ConnectCallback callback) override;

 private:
  void CreateDeviceList(GetDevicesCallback callback,
                        mojom::HidManagerClientAssociatedPtrInfo client,
                        std::vector<mojom::HidDeviceInfoPtr> devices);

  void CreateConnection(ConnectCallback callback,
                        scoped_refptr<HidConnection> connection);

  // HidService::Observer:
  void OnDeviceAdded(mojom::HidDeviceInfoPtr device_info) override;
  void OnDeviceRemoved(mojom::HidDeviceInfoPtr device_info) override;

  std::unique_ptr<HidService> hid_service_;
  mojo::BindingSet<mojom::HidManager> bindings_;
  mojo::AssociatedInterfacePtrSet<mojom::HidManagerClient> clients_;
  ScopedObserver<HidService, HidService::Observer> hid_service_observer_;

  base::WeakPtrFactory<HidManagerImpl> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(HidManagerImpl);
};

}  // namespace device

#endif  // SERVICES_DEVICE_HID_HID_MANAGER_IMPL_H_
