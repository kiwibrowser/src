// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/test/service_observer_mock.h"

namespace audio {

ServiceObserverMock::ServiceObserverMock(
    const std::string& service_name,
    service_manager::mojom::ServiceManagerListenerRequest request)
    : service_name_(service_name), binding_(this, std::move(request)) {}

ServiceObserverMock::~ServiceObserverMock(){};

void ServiceObserverMock::OnInit(
    std::vector<service_manager::mojom::RunningServiceInfoPtr> instances) {
  Initialized();
}

void ServiceObserverMock::OnServiceStarted(
    const service_manager::Identity& identity,
    uint32_t pid) {
  if (identity.name() == service_name_)
    ServiceStarted();
}

void ServiceObserverMock::OnServiceStopped(
    const service_manager::Identity& identity) {
  if (identity.name() == service_name_)
    ServiceStopped();
}

}  // namespace audio
