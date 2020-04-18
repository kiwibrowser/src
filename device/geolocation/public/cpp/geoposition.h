// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_PUBLIC_CPP_GEOPOSITION_H_
#define DEVICE_GEOLOCATION_PUBLIC_CPP_GEOPOSITION_H_

#include "services/device/public/mojom/geoposition.mojom.h"

namespace device {

bool ValidateGeoposition(const mojom::Geoposition& position);

}  // namespace device

#endif  // DEVICE_GEOLOCATION_PUBLIC_CPP_GEOPOSITION_H_
