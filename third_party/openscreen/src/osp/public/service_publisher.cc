// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/service_publisher.h"

namespace openscreen {

ServicePublisherError::ServicePublisherError() = default;
ServicePublisherError::ServicePublisherError(Code error,
                                             const std::string& message)
    : error(error), message(message) {}
ServicePublisherError::ServicePublisherError(
    const ServicePublisherError& other) = default;
ServicePublisherError::~ServicePublisherError() = default;

ServicePublisherError& ServicePublisherError::operator=(
    const ServicePublisherError& other) = default;

ServicePublisher::Metrics::Metrics() = default;
ServicePublisher::Metrics::~Metrics() = default;

ServicePublisher::Config::Config() = default;
ServicePublisher::Config::~Config() = default;

ServicePublisher::ServicePublisher(Observer* observer) : observer_(observer) {}
ServicePublisher::~ServicePublisher() = default;

}  // namespace openscreen
