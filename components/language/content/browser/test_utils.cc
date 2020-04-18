// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/browser/test_utils.h"

namespace language {

MockGeoLocation::MockGeoLocation() : binding_(this) {}
MockGeoLocation::~MockGeoLocation() {}

void MockGeoLocation::SetHighAccuracy(bool high_accuracy) {}

void MockGeoLocation::QueryNextPosition(QueryNextPositionCallback callback) {
  ++query_next_position_called_times_;
  std::move(callback).Run(position_.Clone());
}

void MockGeoLocation::BindGeoLocation(
    device::mojom::GeolocationRequest request) {
  binding_.Bind(std::move(request));
}

void MockGeoLocation::MoveToLocation(float latitude, float longitude) {
  position_.latitude = latitude;
  position_.longitude = longitude;
}

MockIpGeoLocationProvider::MockIpGeoLocationProvider(
    MockGeoLocation* mock_geo_location)
    : mock_geo_location_(mock_geo_location), binding_(this) {}

MockIpGeoLocationProvider::~MockIpGeoLocationProvider() {}

void MockIpGeoLocationProvider::Bind(mojo::ScopedMessagePipeHandle handle) {
  binding_.Bind(device::mojom::PublicIpAddressGeolocationProviderRequest(
      std::move(handle)));
}

void MockIpGeoLocationProvider::CreateGeolocation(
    const net::MutablePartialNetworkTrafficAnnotationTag& /* unused */,
    device::mojom::GeolocationRequest request) {
  mock_geo_location_->BindGeoLocation(std::move(request));
}

}  // namespace language
