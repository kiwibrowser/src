// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/ios/browser/wait_for_network_callback_helper.h"

WaitForNetworkCallbackHelper::WaitForNetworkCallbackHelper() {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

WaitForNetworkCallbackHelper::~WaitForNetworkCallbackHelper() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void WaitForNetworkCallbackHelper::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  if (net::NetworkChangeNotifier::IsOffline())
    return;

  for (const base::Closure& callback : delayed_callbacks_)
    callback.Run();

  delayed_callbacks_.clear();
}

void WaitForNetworkCallbackHelper::HandleCallback(
    const base::Closure& callback) {
  if (net::NetworkChangeNotifier::IsOffline()) {
    delayed_callbacks_.push_back(callback);
  } else {
    callback.Run();
  }
}
