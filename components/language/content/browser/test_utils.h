// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LANGUAGE_CONTENT_BROWSER_TEST_UTILS_H_
#define COMPONENTS_LANGUAGE_CONTENT_BROWSER_TEST_UTILS_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/device/public/mojom/geolocation.mojom.h"
#include "services/device/public/mojom/public_ip_address_geolocation_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/mojom/connector.mojom.h"

namespace language {

// Mock impl of mojom::Geolocation that allows tests to control the returned
// location.
class MockGeoLocation : public device::mojom::Geolocation {
 public:
  MockGeoLocation();
  ~MockGeoLocation() override;

  // device::mojom::Geolocation implementation:
  void SetHighAccuracy(bool high_accuracy) override;
  void QueryNextPosition(QueryNextPositionCallback callback) override;

  void BindGeoLocation(device::mojom::GeolocationRequest request);
  void MoveToLocation(float latitude, float longitude);

  int query_next_position_called_times() const {
    return query_next_position_called_times_;
  }

 private:
  int query_next_position_called_times_ = 0;
  device::mojom::Geoposition position_;
  mojo::Binding<device::mojom::Geolocation> binding_;
};

// Mock impl of mojom::PublicIpAddressGeolocationProvider that binds Geolocation
// to testing impl.
class MockIpGeoLocationProvider
    : public device::mojom::PublicIpAddressGeolocationProvider {
 public:
  explicit MockIpGeoLocationProvider(MockGeoLocation* mock_geo_location);
  ~MockIpGeoLocationProvider() override;

  void Bind(mojo::ScopedMessagePipeHandle handle);

  void CreateGeolocation(
      const net::MutablePartialNetworkTrafficAnnotationTag& /* unused */,
      device::mojom::GeolocationRequest request) override;

 private:
  MockGeoLocation* mock_geo_location_;
  mojo::Binding<device::mojom::PublicIpAddressGeolocationProvider> binding_;
};

}  // namespace language

#endif  // COMPONENTS_LANGUAGE_CONTENT_BROWSER_TEST_UTILS_H_
