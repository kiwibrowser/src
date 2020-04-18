// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_PUBLIC_CPP_SCOPED_GEOLOCATION_OVERRIDER_H_
#define DEVICE_GEOLOCATION_PUBLIC_CPP_SCOPED_GEOLOCATION_OVERRIDER_H_

#include "base/bind.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/device/public/mojom/geolocation.mojom.h"
#include "services/device/public/mojom/geolocation_context.mojom.h"
#include "services/device/public/mojom/geoposition.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace device {

// A helper class which owns a FakeGeolocationContext by which the geolocation
// is overriden to a given position or latitude and longitude values.
// The FakeGeolocationContext overrides the binder of Device Service by
// service_manager::ServiceContext::SetGlobalBinderForTesting().
// The override of the geolocation implementation will be in effect for the
// duration of this object's lifetime.
class ScopedGeolocationOverrider {
 public:
  explicit ScopedGeolocationOverrider(const mojom::Geoposition& position);
  ScopedGeolocationOverrider(double latitude, double longitude);
  ~ScopedGeolocationOverrider();
  void OverrideGeolocation(const mojom::Geoposition& position);
  void UpdateLocation(const mojom::Geoposition& position);
  void UpdateLocation(double latitude, double longitude);

 private:
  class FakeGeolocation;
  class FakeGeolocationContext;
  std::unique_ptr<FakeGeolocationContext> geolocation_context_;
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_PUBLIC_CPP_SCOPED_GEOLOCATION_OVERRIDER_H_
