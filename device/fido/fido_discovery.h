// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_DISCOVERY_H_
#define DEVICE_FIDO_FIDO_DISCOVERY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_piece.h"
#include "device/fido/fido_transport_protocol.h"

namespace service_manager {
class Connector;
}

namespace device {

class FidoDevice;

namespace internal {
class ScopedFidoDiscoveryFactory;
}

class COMPONENT_EXPORT(DEVICE_FIDO) FidoDiscovery {
 public:
  enum class State {
    kIdle,
    kStarting,
    kRunning,
  };

  class COMPONENT_EXPORT(DEVICE_FIDO) Observer {
   public:
    virtual ~Observer();

    // It is guaranteed that this is never invoked synchronously from Start().
    virtual void DiscoveryStarted(FidoDiscovery* discovery, bool success) = 0;

    // It is guaranteed that DeviceAdded/DeviceRemoved() will not be invoked
    // before the client of FidoDiscovery calls FidoDiscovery::Start(). However,
    // for devices already known to the system at that point, DeviceAdded()
    // might already be called to reported already known devices.
    virtual void DeviceAdded(FidoDiscovery* discovery, FidoDevice* device) = 0;
    virtual void DeviceRemoved(FidoDiscovery* discovery,
                               FidoDevice* device) = 0;
  };

  // Factory function to construct an instance that discovers authenticators on
  // the given |transport| protocol.
  //
  // FidoTransportProtocol::kUsbHumanInterfaceDevice requires specifying a valid
  // |connector| on Desktop, and is not valid on Android.
  static std::unique_ptr<FidoDiscovery> Create(
      FidoTransportProtocol transport,
      ::service_manager::Connector* connector);

  virtual ~FidoDiscovery();

  Observer* observer() const { return observer_; }
  void set_observer(Observer* observer) {
    DCHECK(!observer_ || !observer) << "Only one observer is supported.";
    observer_ = observer;
  }

  FidoTransportProtocol transport() const { return transport_; }
  bool is_start_requested() const { return state_ != State::kIdle; }
  bool is_running() const { return state_ == State::kRunning; }

  void Start();

  std::vector<FidoDevice*> GetDevices();
  std::vector<const FidoDevice*> GetDevices() const;

  FidoDevice* GetDevice(base::StringPiece device_id);
  const FidoDevice* GetDevice(base::StringPiece device_id) const;

 protected:
  FidoDiscovery(FidoTransportProtocol transport);

  void NotifyDiscoveryStarted(bool success);
  void NotifyDeviceAdded(FidoDevice* device);
  void NotifyDeviceRemoved(FidoDevice* device);

  bool AddDevice(std::unique_ptr<FidoDevice> device);
  bool RemoveDevice(base::StringPiece device_id);

  // Subclasses should implement this to actually start the discovery when it is
  // requested.
  //
  // The implementation should asynchronously invoke NotifyDiscoveryStarted when
  // the discovery is s tarted.
  virtual void StartInternal() = 0;

  std::map<std::string, std::unique_ptr<FidoDevice>, std::less<>> devices_;
  Observer* observer_ = nullptr;

 private:
  friend class internal::ScopedFidoDiscoveryFactory;

  // Factory function can be overridden by tests to construct fakes.
  using FactoryFuncPtr = decltype(&Create);
  static FactoryFuncPtr g_factory_func_;

  const FidoTransportProtocol transport_;
  State state_ = State::kIdle;

  DISALLOW_COPY_AND_ASSIGN(FidoDiscovery);
};

namespace internal {

// Base class for a scoped override of FidoDiscovery::Create, used in unit
// tests, layout tests, and when running with the Web Authn Testing API enabled.
//
// While there is a subclass instance in scope, calls to the factory method will
// be hijacked such that the derived class's CreateFidoDiscovery method will be
// invoked instead.
class COMPONENT_EXPORT(DEVICE_FIDO) ScopedFidoDiscoveryFactory {
 public:
  // There should be at most one instance of any subclass in scope at a time.
  ScopedFidoDiscoveryFactory();
  virtual ~ScopedFidoDiscoveryFactory();

 protected:
  virtual std::unique_ptr<FidoDiscovery> CreateFidoDiscovery(
      FidoTransportProtocol transport,
      ::service_manager::Connector* connector) = 0;

 private:
  static std::unique_ptr<FidoDiscovery>
  ForwardCreateFidoDiscoveryToCurrentFactory(
      FidoTransportProtocol transport,
      ::service_manager::Connector* connector);

  static ScopedFidoDiscoveryFactory* g_current_factory;
  FidoDiscovery::FactoryFuncPtr original_factory_func_;

  DISALLOW_COPY_AND_ASSIGN(ScopedFidoDiscoveryFactory);
};

}  // namespace internal
}  // namespace device

#endif  // DEVICE_FIDO_FIDO_DISCOVERY_H_
