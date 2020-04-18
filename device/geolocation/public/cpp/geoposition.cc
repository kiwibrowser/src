// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/geolocation/public/cpp/geoposition.h"

namespace device {

bool ValidateGeoposition(const mojom::Geoposition& position) {
  return position.latitude >= -90. && position.latitude <= 90. &&
         position.longitude >= -180. && position.longitude <= 180. &&
         position.accuracy >= 0. && !position.timestamp.is_null();
}

}  // namespace device
