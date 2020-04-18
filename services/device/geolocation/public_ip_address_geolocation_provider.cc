// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_geolocation_provider.h"

#include "services/device/geolocation/public_ip_address_geolocator.h"

namespace device {

PublicIpAddressGeolocationProvider::PublicIpAddressGeolocationProvider(
    GeolocationProvider::RequestContextProducer request_context_producer,
    const std::string& api_key) {
  // Bind sequence_checker_ to the initialization sequence.
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  public_ip_address_location_notifier_ =
      std::make_unique<PublicIpAddressLocationNotifier>(
          request_context_producer, api_key);
}

PublicIpAddressGeolocationProvider::~PublicIpAddressGeolocationProvider() {}

void PublicIpAddressGeolocationProvider::Bind(
    mojom::PublicIpAddressGeolocationProviderRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(public_ip_address_location_notifier_);
  provider_binding_set_.AddBinding(this, std::move(request));
}

void PublicIpAddressGeolocationProvider::CreateGeolocation(
    const net::MutablePartialNetworkTrafficAnnotationTag& tag,
    mojom::GeolocationRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(public_ip_address_location_notifier_);
  geolocation_binding_set_.AddBinding(
      std::make_unique<PublicIpAddressGeolocator>(
          static_cast<net::PartialNetworkTrafficAnnotationTag>(tag),
          public_ip_address_location_notifier_.get(),
          base::Bind(
              &mojo::StrongBindingSet<mojom::Geolocation>::ReportBadMessage,
              base::Unretained(&geolocation_binding_set_))),
      std::move(request));
}

}  // namespace device
