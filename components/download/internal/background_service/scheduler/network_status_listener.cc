// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/background_service/scheduler/network_status_listener.h"

namespace download {

NetworkStatusListener::NetworkStatusListener() = default;

void NetworkStatusListener::Start(NetworkStatusListener::Observer* observer) {
  observer_ = observer;
}

void NetworkStatusListener::Stop() {
  observer_ = nullptr;
}

NetworkStatusListenerImpl::NetworkStatusListenerImpl() = default;

NetworkStatusListenerImpl::~NetworkStatusListenerImpl() = default;

void NetworkStatusListenerImpl::Start(
    NetworkStatusListener::Observer* observer) {
  NetworkStatusListener::Start(observer);
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

void NetworkStatusListenerImpl::Stop() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
  NetworkStatusListener::Stop();
}

net::NetworkChangeNotifier::ConnectionType
NetworkStatusListenerImpl::GetConnectionType() {
  return net::NetworkChangeNotifier::GetConnectionType();
}

void NetworkStatusListenerImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  DCHECK(observer_);
  observer_->OnNetworkChanged(type);
}

}  // namespace download
