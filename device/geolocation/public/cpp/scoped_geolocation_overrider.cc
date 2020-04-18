// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "device/geolocation/public/cpp/geoposition.h"
#include "device/geolocation/public/cpp/scoped_geolocation_overrider.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace device {

// This class is a fake implementation of GeolocationContext and Geolocation
// mojo interfaces for those tests which want to set an override geoposition
// value and verify their code where there are geolocation mojo calls.
class ScopedGeolocationOverrider::FakeGeolocationContext
    : public mojom::GeolocationContext {
 public:
  explicit FakeGeolocationContext(const mojom::Geoposition& position);
  ~FakeGeolocationContext() override;

  void UpdateLocation(const mojom::Geoposition& position);
  const mojom::Geoposition& GetGeoposition() const;

  void BindForOverrideService(
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle handle,
      const service_manager::BindSourceInfo& source_info);

  // mojom::GeolocationContext implementation:
  void BindGeolocation(mojom::GeolocationRequest request) override;
  void SetOverride(mojom::GeopositionPtr geoposition) override;
  void ClearOverride() override;

 private:
  mojom::Geoposition position_;
  mojom::GeopositionPtr override_position_;
  std::vector<std::unique_ptr<FakeGeolocation>> impls_;
  mojo::BindingSet<mojom::GeolocationContext> context_bindings_;
};

class ScopedGeolocationOverrider::FakeGeolocation : public mojom::Geolocation {
 public:
  FakeGeolocation(mojom::GeolocationRequest request,
                  const FakeGeolocationContext* context);
  ~FakeGeolocation() override;

  void UpdateLocation(const mojom::Geoposition& position);

  // mojom::Geolocation implementation:
  void QueryNextPosition(QueryNextPositionCallback callback) override;
  void SetHighAccuracy(bool high_accuracy) override;

 private:
  const FakeGeolocationContext* context_;
  bool has_new_position_;
  QueryNextPositionCallback position_callback_;
  mojo::Binding<mojom::Geolocation> binding_;
};

ScopedGeolocationOverrider::ScopedGeolocationOverrider(
    const mojom::Geoposition& position) {
  OverrideGeolocation(position);
}

ScopedGeolocationOverrider::ScopedGeolocationOverrider(double latitude,
                                                       double longitude) {
  mojom::Geoposition position;
  position.latitude = latitude;
  position.longitude = longitude;
  position.altitude = 0.;
  position.accuracy = 0.;
  position.timestamp = base::Time::Now();

  OverrideGeolocation(position);
}

ScopedGeolocationOverrider::~ScopedGeolocationOverrider() {
  service_manager::ServiceContext::ClearGlobalBindersForTesting(
      mojom::kServiceName);
}

void ScopedGeolocationOverrider::OverrideGeolocation(
    const mojom::Geoposition& position) {
  geolocation_context_ = std::make_unique<FakeGeolocationContext>(position);
  service_manager::ServiceContext::SetGlobalBinderForTesting(
      mojom::kServiceName, mojom::GeolocationContext::Name_,
      base::BindRepeating(&FakeGeolocationContext::BindForOverrideService,
                          base::Unretained(geolocation_context_.get())));
}

void ScopedGeolocationOverrider::UpdateLocation(
    const mojom::Geoposition& position) {
  geolocation_context_->UpdateLocation(position);
}

void ScopedGeolocationOverrider::UpdateLocation(double latitude,
                                                double longitude) {
  mojom::Geoposition position;
  position.latitude = latitude;
  position.longitude = longitude;
  position.altitude = 0.;
  position.accuracy = 0.;
  position.timestamp = base::Time::Now();

  UpdateLocation(position);
}

ScopedGeolocationOverrider::FakeGeolocationContext::FakeGeolocationContext(
    const mojom::Geoposition& position)
    : position_(position) {
  position_.valid = false;
  if (ValidateGeoposition(position_))
    position_.valid = true;
}

ScopedGeolocationOverrider::FakeGeolocationContext::~FakeGeolocationContext() {}

void ScopedGeolocationOverrider::FakeGeolocationContext::UpdateLocation(
    const mojom::Geoposition& position) {
  position_ = position;

  position_.valid = false;
  if (ValidateGeoposition(position_))
    position_.valid = true;

  for (auto& impl : impls_) {
    impl->UpdateLocation(position_);
  }
}

const mojom::Geoposition&
ScopedGeolocationOverrider::FakeGeolocationContext::GetGeoposition() const {
  if (!override_position_.is_null())
    return *override_position_;

  return position_;
}

void ScopedGeolocationOverrider::FakeGeolocationContext::BindForOverrideService(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle handle,
    const service_manager::BindSourceInfo& source_info) {
  context_bindings_.AddBinding(
      this, mojom::GeolocationContextRequest(std::move(handle)));
}

void ScopedGeolocationOverrider::FakeGeolocationContext::BindGeolocation(
    mojom::GeolocationRequest request) {
  impls_.push_back(std::make_unique<FakeGeolocation>(std::move(request), this));
}

void ScopedGeolocationOverrider::FakeGeolocationContext::SetOverride(
    mojom::GeopositionPtr geoposition) {
  override_position_ = std::move(geoposition);
  if (override_position_.is_null())
    return;

  override_position_->valid = false;
  if (ValidateGeoposition(*override_position_))
    override_position_->valid = true;

  for (auto& impl : impls_) {
    impl->UpdateLocation(*override_position_);
  }
}

void ScopedGeolocationOverrider::FakeGeolocationContext::ClearOverride() {
  override_position_.reset();
}

ScopedGeolocationOverrider::FakeGeolocation::FakeGeolocation(
    mojom::GeolocationRequest request,
    const FakeGeolocationContext* context)
    : context_(context), has_new_position_(true), binding_(this) {
  binding_.Bind(std::move(request));
}

ScopedGeolocationOverrider::FakeGeolocation::~FakeGeolocation() {}

void ScopedGeolocationOverrider::FakeGeolocation::UpdateLocation(
    const mojom::Geoposition& position) {
  has_new_position_ = true;
  if (!position_callback_.is_null()) {
    std::move(position_callback_).Run(position.Clone());
    has_new_position_ = false;
  }
}

void ScopedGeolocationOverrider::FakeGeolocation::QueryNextPosition(
    QueryNextPositionCallback callback) {
  // Pending callbacks might be overrided.
  position_callback_ = std::move(callback);

  if (has_new_position_) {
    std::move(position_callback_).Run(context_->GetGeoposition().Clone());
    has_new_position_ = false;
  }
}

void ScopedGeolocationOverrider::FakeGeolocation::SetHighAccuracy(
    bool high_accuracy) {}

}  // namespace device
