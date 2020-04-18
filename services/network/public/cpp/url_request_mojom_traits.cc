// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/url_request_mojom_traits.h"

#include "base/logging.h"

namespace mojo {

network::mojom::RequestPriority
EnumTraits<network::mojom::RequestPriority, net::RequestPriority>::ToMojom(
    net::RequestPriority priority) {
  switch (priority) {
    case net::THROTTLED:
      return network::mojom::RequestPriority::kThrottled;
    case net::IDLE:
      return network::mojom::RequestPriority::kIdle;
    case net::LOWEST:
      return network::mojom::RequestPriority::kLowest;
    case net::LOW:
      return network::mojom::RequestPriority::kLow;
    case net::MEDIUM:
      return network::mojom::RequestPriority::kMedium;
    case net::HIGHEST:
      return network::mojom::RequestPriority::kHighest;
  }
  NOTREACHED();
  return static_cast<network::mojom::RequestPriority>(priority);
}

bool EnumTraits<network::mojom::RequestPriority, net::RequestPriority>::
    FromMojom(network::mojom::RequestPriority in, net::RequestPriority* out) {
  switch (in) {
    case network::mojom::RequestPriority::kThrottled:
      *out = net::THROTTLED;
      return true;
    case network::mojom::RequestPriority::kIdle:
      *out = net::IDLE;
      return true;
    case network::mojom::RequestPriority::kLowest:
      *out = net::LOWEST;
      return true;
    case network::mojom::RequestPriority::kLow:
      *out = net::LOW;
      return true;
    case network::mojom::RequestPriority::kMedium:
      *out = net::MEDIUM;
      return true;
    case network::mojom::RequestPriority::kHighest:
      *out = net::HIGHEST;
      return true;
  }

  NOTREACHED();
  *out = static_cast<net::RequestPriority>(in);
  return true;
}

}  // namespace mojo
