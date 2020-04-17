// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/service_listener.h"

namespace openscreen {

ServiceListenerError::ServiceListenerError() = default;
ServiceListenerError::ServiceListenerError(Code error,
                                           const std::string& message)
    : error(error), message(message) {}
ServiceListenerError::ServiceListenerError(const ServiceListenerError& other) =
    default;
ServiceListenerError::~ServiceListenerError() = default;

ServiceListenerError& ServiceListenerError::operator=(
    const ServiceListenerError& other) = default;

ServiceListener::Metrics::Metrics() = default;
ServiceListener::Metrics::~Metrics() = default;

ServiceListener::ServiceListener() = default;
ServiceListener::~ServiceListener() = default;

}  // namespace openscreen
