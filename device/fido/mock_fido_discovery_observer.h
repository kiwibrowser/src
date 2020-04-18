// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_MOCK_FIDO_DISCOVERY_OBSERVER_H_
#define DEVICE_FIDO_MOCK_FIDO_DISCOVERY_OBSERVER_H_

#include "base/component_export.h"
#include "base/macros.h"
#include "device/fido/fido_discovery.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace device {

class FidoDevice;

class MockFidoDiscoveryObserver : public FidoDiscovery::Observer {
 public:
  MockFidoDiscoveryObserver();
  ~MockFidoDiscoveryObserver() override;

  MOCK_METHOD2(DiscoveryStarted, void(FidoDiscovery*, bool));
  MOCK_METHOD2(DiscoveryStopped, void(FidoDiscovery*, bool));
  MOCK_METHOD2(DeviceAdded, void(FidoDiscovery*, FidoDevice*));
  MOCK_METHOD2(DeviceRemoved, void(FidoDiscovery*, FidoDevice*));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockFidoDiscoveryObserver);
};

}  // namespace device

#endif  // DEVICE_FIDO_MOCK_FIDO_DISCOVERY_OBSERVER_H_
