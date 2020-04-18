// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/geolocation/geolocation_provider_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "device/geolocation/location_arbitrator.h"
#include "device/geolocation/public/cpp/geoposition.h"

namespace device {

namespace {
base::LazyInstance<CustomLocationProviderCallback>::Leaky
    g_custom_location_provider_callback = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<GeolocationProvider::RequestContextProducer>::Leaky
    g_request_context_producer = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<std::string>::Leaky g_api_key = LAZY_INSTANCE_INITIALIZER;
}  // namespace

// static
GeolocationProvider* GeolocationProvider::GetInstance() {
  return GeolocationProviderImpl::GetInstance();
}

// static
void GeolocationProviderImpl::SetGeolocationGlobals(
    const GeolocationProvider::RequestContextProducer request_context_producer,
    const std::string& api_key,
    const CustomLocationProviderCallback& callback) {
  g_request_context_producer.Get() = request_context_producer;
  g_api_key.Get() = api_key;
  g_custom_location_provider_callback.Get() = callback;
}

std::unique_ptr<GeolocationProvider::Subscription>
GeolocationProviderImpl::AddLocationUpdateCallback(
    const LocationUpdateCallback& callback,
    bool enable_high_accuracy) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::unique_ptr<GeolocationProvider::Subscription> subscription;
  if (enable_high_accuracy) {
    subscription = high_accuracy_callbacks_.Add(callback);
  } else {
    subscription = low_accuracy_callbacks_.Add(callback);
  }

  OnClientsChanged();
  if (ValidateGeoposition(position_) ||
      position_.error_code != mojom::Geoposition::ErrorCode::NONE) {
    callback.Run(position_);
  }

  return subscription;
}

bool GeolocationProviderImpl::HighAccuracyLocationInUse() {
  return !high_accuracy_callbacks_.empty();
}

void GeolocationProviderImpl::OverrideLocationForTesting(
    const mojom::Geoposition& position) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  ignore_location_updates_ = true;
  NotifyClients(position);
}

void GeolocationProviderImpl::OnLocationUpdate(
    const LocationProvider* provider,
    const mojom::Geoposition& position) {
  DCHECK(OnGeolocationThread());
  // Will be true only in testing.
  if (ignore_location_updates_)
    return;
  main_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&GeolocationProviderImpl::NotifyClients,
                                base::Unretained(this), position));
}

// static
GeolocationProviderImpl* GeolocationProviderImpl::GetInstance() {
  return base::Singleton<GeolocationProviderImpl>::get();
}

void GeolocationProviderImpl::BindGeolocationControlRequest(
    mojom::GeolocationControlRequest request) {
  // The |binding_| has been bound already here means that more than one
  // GeolocationPermissionContext in chrome tried to bind to Device Service.
  // We only bind the first request. See more info in geolocation_control.mojom.
  if (!binding_.is_bound())
    binding_.Bind(std::move(request));
}

void GeolocationProviderImpl::UserDidOptIntoLocationServices() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool was_permission_granted = user_did_opt_into_location_services_;
  user_did_opt_into_location_services_ = true;
  if (IsRunning() && !was_permission_granted)
    InformProvidersPermissionGranted();
}

GeolocationProviderImpl::GeolocationProviderImpl()
    : base::Thread("Geolocation"),
      user_did_opt_into_location_services_(false),
      ignore_location_updates_(false),
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      binding_(this) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  high_accuracy_callbacks_.set_removal_callback(base::Bind(
      &GeolocationProviderImpl::OnClientsChanged, base::Unretained(this)));
  low_accuracy_callbacks_.set_removal_callback(base::Bind(
      &GeolocationProviderImpl::OnClientsChanged, base::Unretained(this)));
}

GeolocationProviderImpl::~GeolocationProviderImpl() {
  Stop();
  DCHECK(!arbitrator_);
}

void GeolocationProviderImpl::SetArbitratorForTesting(
    std::unique_ptr<LocationProvider> arbitrator) {
  arbitrator_ = std::move(arbitrator);
}

bool GeolocationProviderImpl::OnGeolocationThread() const {
  return task_runner()->BelongsToCurrentThread();
}

void GeolocationProviderImpl::OnClientsChanged() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  base::Closure task;
  if (high_accuracy_callbacks_.empty() && low_accuracy_callbacks_.empty()) {
    DCHECK(IsRunning());
    if (!ignore_location_updates_) {
      // We have no more observers, so we clear the cached geoposition so that
      // when the next observer is added we will not provide a stale position.
      position_ = mojom::Geoposition();
    }
    task = base::Bind(&GeolocationProviderImpl::StopProviders,
                      base::Unretained(this));
  } else {
    if (!IsRunning()) {
      Start();
      if (user_did_opt_into_location_services_)
        InformProvidersPermissionGranted();
    }
    // Determine a set of options that satisfies all clients.
    bool enable_high_accuracy = !high_accuracy_callbacks_.empty();

    // Send the current options to the providers as they may have changed.
    task = base::Bind(&GeolocationProviderImpl::StartProviders,
                      base::Unretained(this), enable_high_accuracy);
  }

  task_runner()->PostTask(FROM_HERE, task);
}

void GeolocationProviderImpl::StopProviders() {
  DCHECK(OnGeolocationThread());
  DCHECK(arbitrator_);
  arbitrator_->StopProvider();
}

void GeolocationProviderImpl::StartProviders(bool enable_high_accuracy) {
  DCHECK(OnGeolocationThread());
  DCHECK(arbitrator_);
  arbitrator_->StartProvider(enable_high_accuracy);
}

void GeolocationProviderImpl::InformProvidersPermissionGranted() {
  DCHECK(IsRunning());
  if (!OnGeolocationThread()) {
    task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &GeolocationProviderImpl::InformProvidersPermissionGranted,
            base::Unretained(this)));
    return;
  }
  DCHECK(OnGeolocationThread());
  DCHECK(arbitrator_);
  arbitrator_->OnPermissionGranted();
}

void GeolocationProviderImpl::NotifyClients(
    const mojom::Geoposition& position) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK(ValidateGeoposition(position) ||
         position.error_code != mojom::Geoposition::ErrorCode::NONE);
  position_ = position;
  high_accuracy_callbacks_.Notify(position_);
  low_accuracy_callbacks_.Notify(position_);
}

void GeolocationProviderImpl::Init() {
  DCHECK(OnGeolocationThread());

  if (arbitrator_)
    return;

  LocationProvider::LocationProviderUpdateCallback callback = base::Bind(
      &GeolocationProviderImpl::OnLocationUpdate, base::Unretained(this));

  arbitrator_ = std::make_unique<LocationArbitrator>(
      g_custom_location_provider_callback.Get(),
      g_request_context_producer.Get(), g_api_key.Get());
  arbitrator_->SetUpdateCallback(callback);
}

void GeolocationProviderImpl::CleanUp() {
  DCHECK(OnGeolocationThread());
  arbitrator_.reset();
}

}  // namespace device
