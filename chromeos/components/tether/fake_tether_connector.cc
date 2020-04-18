// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/fake_tether_connector.h"

namespace chromeos {

namespace tether {

FakeTetherConnector::FakeTetherConnector() = default;

FakeTetherConnector::~FakeTetherConnector() = default;

void FakeTetherConnector::ConnectToNetwork(
    const std::string& tether_network_guid,
    const base::Closure& success_callback,
    const network_handler::StringResultCallback& error_callback) {
  last_connected_tether_network_guid_ = tether_network_guid;

  if (connection_error_name_.empty())
    success_callback.Run();
  else
    error_callback.Run(connection_error_name_);
}

bool FakeTetherConnector::CancelConnectionAttempt(
    const std::string& tether_network_guid) {
  last_canceled_tether_network_guid_ = tether_network_guid;
  return should_cancel_successfully_;
}

}  // namespace tether

}  // namespace chromeos
