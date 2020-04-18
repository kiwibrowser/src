// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_DEVICE_BLUETOOTH_LE_GATT_CLIENT_MANAGER_H_
#define CHROMECAST_DEVICE_BLUETOOTH_LE_GATT_CLIENT_MANAGER_H_

#include <map>
#include <set>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "chromecast/public/bluetooth/bluetooth_types.h"

namespace chromecast {
namespace bluetooth {

class RemoteCharacteristic;
class RemoteDevice;
class RemoteService;

class GattClientManager {
 public:
  class Observer {
   public:
    // Called when the connection state changes for |device|.
    virtual void OnConnectChanged(scoped_refptr<RemoteDevice> device,
                                  bool connected) {}

    // Called when the connection MTU changes for |device|.
    virtual void OnMtuChanged(scoped_refptr<RemoteDevice> device, int mtu) {}

    // Called when the device |device|'s service list changed. |services| is the
    // new list of services, and any old ones should be considered invalidated.
    virtual void OnServicesUpdated(
        scoped_refptr<RemoteDevice> device,
        std::vector<scoped_refptr<RemoteService>> services) {}

    // Called when |device| has a notification on |characteristic| with |value|.
    virtual void OnCharacteristicNotification(
        scoped_refptr<RemoteDevice> device,
        scoped_refptr<RemoteCharacteristic> characteristic,
        std::vector<uint8_t> value) {}

    // Called when a connection is requested.
    virtual void OnConnectInitated(const bluetooth_v2_shlib::Addr& addr) {}

    virtual ~Observer() = default;
  };

  virtual void AddObserver(Observer* o) = 0;
  virtual void RemoveObserver(Observer* o) = 0;

  // TODO(bcf/slan): Add new method:
  // void GetDevices(Callback<void(vector<scoped_refptr<RemoteDevice>>)> cb);

  // Get a RemoteDevice object corresponding to |addr| for performing GATT
  // operations. |cb| will be run on the callers thread. Callbacks passed into
  // methods on RemoteDevice and its subobjects (RemoteService,
  // RemoteCharacteristic, RemoteDescriptor) will also be run on the thread
  // which called the specific method.
  virtual void GetDevice(
      const bluetooth_v2_shlib::Addr& addr,
      base::OnceCallback<void(scoped_refptr<RemoteDevice>)> cb) = 0;

  // TODO(bcf): Deprecated. Replace usage with async version.
  virtual scoped_refptr<RemoteDevice> GetDeviceSync(
      const bluetooth_v2_shlib::Addr& addr) = 0;

  // TODO(bcf): Make async.
  // Returns the number of devices which are currently connected.
  virtual size_t GetNumConnected() const = 0;

  virtual void NotifyConnect(const bluetooth_v2_shlib::Addr& addr) = 0;

  // TODO(bcf): Deprecated. Should be removed now that this class may be used
  // from any thread.
  virtual scoped_refptr<base::SingleThreadTaskRunner> task_runner() = 0;

 protected:
  GattClientManager() = default;
  virtual ~GattClientManager() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(GattClientManager);
};

}  // namespace bluetooth
}  // namespace chromecast

#endif  // CHROMECAST_DEVICE_BLUETOOTH_LE_GATT_CLIENT_MANAGER_H_
