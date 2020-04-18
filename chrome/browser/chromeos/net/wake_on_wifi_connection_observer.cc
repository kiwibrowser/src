// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/net/wake_on_wifi_connection_observer.h"


#include "base/logging.h"
#include "chrome/browser/chromeos/net/wake_on_wifi_manager.h"
#include "chrome/browser/gcm/gcm_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/network/network_device_handler.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/gcm_driver/gcm_profile_service.h"

namespace chromeos {

WakeOnWifiConnectionObserver::WakeOnWifiConnectionObserver(
    Profile* profile,
    bool wifi_properties_received,
    WakeOnWifiManager::WakeOnWifiFeature feature,
    NetworkDeviceHandler* network_device_handler)
    : profile_(profile),
      ip_endpoint_(net::IPEndPoint()),
      wifi_properties_received_(wifi_properties_received),
      feature_(feature),
      network_device_handler_(network_device_handler) {
  gcm::GCMProfileServiceFactory::GetForProfile(profile_)
      ->driver()
      ->AddConnectionObserver(this);
}

WakeOnWifiConnectionObserver::~WakeOnWifiConnectionObserver() {
  if (!(ip_endpoint_ == net::IPEndPoint()))
    OnDisconnected();

  gcm::GCMProfileServiceFactory::GetForProfile(profile_)
      ->driver()
      ->RemoveConnectionObserver(this);
}

void WakeOnWifiConnectionObserver::HandleWifiDevicePropertiesReady() {
  wifi_properties_received_ = true;

  // IPEndPoint doesn't implement operator!=
  if (ip_endpoint_ == net::IPEndPoint())
    return;

  AddWakeOnPacketConnection();
}

void WakeOnWifiConnectionObserver::OnConnected(
    const net::IPEndPoint& ip_endpoint) {
  ip_endpoint_ = ip_endpoint;

  if (wifi_properties_received_)
    AddWakeOnPacketConnection();
}

void WakeOnWifiConnectionObserver::OnDisconnected() {
  if (ip_endpoint_ == net::IPEndPoint()) {
    VLOG(1) << "Received GCMConnectionObserver::OnDisconnected without a "
            << "valid IPEndPoint.";
    return;
  }

  if (wifi_properties_received_)
    RemoveWakeOnPacketConnection();

  ip_endpoint_ = net::IPEndPoint();
}

void WakeOnWifiConnectionObserver::AddWakeOnPacketConnection() {
  if (!WakeOnWifiManager::IsWakeOnPacketEnabled(feature_))
    return;
  network_device_handler_->AddWifiWakeOnPacketConnection(
      ip_endpoint_, base::DoNothing(), network_handler::ErrorCallback());
}

void WakeOnWifiConnectionObserver::RemoveWakeOnPacketConnection() {
  if (!WakeOnWifiManager::IsWakeOnPacketEnabled(feature_))
    return;
  network_device_handler_->RemoveWifiWakeOnPacketConnection(
      ip_endpoint_, base::DoNothing(), network_handler::ErrorCallback());
}

} // namespace chromeos
