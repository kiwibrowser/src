// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/fake_asynchronous_shutdown_object_container.h"

namespace chromeos {

namespace tether {

FakeAsynchronousShutdownObjectContainer::
    FakeAsynchronousShutdownObjectContainer(
        const base::Closure& deletion_callback)
    : deletion_callback_(deletion_callback) {}

FakeAsynchronousShutdownObjectContainer::
    ~FakeAsynchronousShutdownObjectContainer() {
  deletion_callback_.Run();
}

void FakeAsynchronousShutdownObjectContainer::Shutdown(
    const base::Closure& shutdown_complete_callback) {
  shutdown_complete_callback_ = shutdown_complete_callback;
}

TetherHostFetcher*
FakeAsynchronousShutdownObjectContainer::tether_host_fetcher() {
  return tether_host_fetcher_;
}

BleConnectionManager*
FakeAsynchronousShutdownObjectContainer::ble_connection_manager() {
  return ble_connection_manager_;
}

DisconnectTetheringRequestSender*
FakeAsynchronousShutdownObjectContainer::disconnect_tethering_request_sender() {
  return disconnect_tethering_request_sender_;
}

NetworkConfigurationRemover*
FakeAsynchronousShutdownObjectContainer::network_configuration_remover() {
  return network_configuration_remover_;
}

WifiHotspotDisconnector*
FakeAsynchronousShutdownObjectContainer::wifi_hotspot_disconnector() {
  return wifi_hotspot_disconnector_;
}

}  // namespace tether

}  // namespace chromeos
