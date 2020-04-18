// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_GEOLOCATION_PROVIDER_IMPL_H_
#define DEVICE_GEOLOCATION_GEOLOCATION_PROVIDER_IMPL_H_

#include <list>
#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/threading/thread.h"
#include "device/geolocation/geolocation_export.h"
#include "device/geolocation/geolocation_provider.h"
#include "device/geolocation/public/cpp/location_provider.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/public/mojom/geolocation_control.mojom.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
class SingleThreadTaskRunner;
}

namespace device {

// Callback that returns the embedder's custom location provider. This callback
// is provided to the Device Service by its embedder.
using CustomLocationProviderCallback =
    base::Callback<std::unique_ptr<LocationProvider>()>;

class DEVICE_GEOLOCATION_EXPORT GeolocationProviderImpl
    : public GeolocationProvider,
      public mojom::GeolocationControl,
      public base::Thread {
 public:
  // GeolocationProvider implementation:
  std::unique_ptr<GeolocationProvider::Subscription> AddLocationUpdateCallback(
      const LocationUpdateCallback& callback,
      bool enable_high_accuracy) override;
  bool HighAccuracyLocationInUse() override;
  void OverrideLocationForTesting(const mojom::Geoposition& position) override;

  // Callback from the LocationArbitrator. Public for testing.
  void OnLocationUpdate(const LocationProvider* provider,
                        const mojom::Geoposition& position);

  // Gets a pointer to the singleton instance of the location relayer, which
  // is in turn bound to the browser's global context objects. This must only be
  // called on the UI thread so that the GeolocationProviderImpl is always
  // instantiated on the same thread. Ownership is NOT returned.
  static GeolocationProviderImpl* GetInstance();

  // Optional: Provide a callback to produce a request context for network
  // geolocation requests. Provide a Google API key for network geolocation
  // requests. Provide a callback which can return a custom location provider
  // from embedder. Call before using Init() on the singleton GetInstance().
  static void SetGeolocationGlobals(
      const GeolocationProvider::RequestContextProducer
          request_context_producer,
      const std::string& api_key,
      const CustomLocationProviderCallback& callback);

  void BindGeolocationControlRequest(mojom::GeolocationControlRequest request);

  // mojom::GeolocationControl implementation:
  void UserDidOptIntoLocationServices() override;

  bool user_did_opt_into_location_services_for_testing() {
    return user_did_opt_into_location_services_;
  }

  // Safe to call while there are no GeolocationProviderImpl clients
  // registered.
  void SetArbitratorForTesting(std::unique_ptr<LocationProvider> arbitrator);

 private:
  friend struct base::DefaultSingletonTraits<GeolocationProviderImpl>;
  GeolocationProviderImpl();
  ~GeolocationProviderImpl() override;

  bool OnGeolocationThread() const;

  // Start and stop providers as needed when clients are added or removed.
  void OnClientsChanged();

  // Stops the providers when there are no more registered clients. Note that
  // once the Geolocation thread is started, it will stay alive (but sitting
  // idle without any pending messages).
  void StopProviders();

  // Starts the geolocation providers or updates their options (delegates to
  // arbitrator).
  void StartProviders(bool enable_high_accuracy);

  // Updates the providers on the geolocation thread, which must be running.
  void InformProvidersPermissionGranted();

  // Notifies all registered clients that a position update is available.
  void NotifyClients(const mojom::Geoposition& position);

  // Thread
  void Init() override;
  void CleanUp() override;

  base::CallbackList<void(const mojom::Geoposition&)> high_accuracy_callbacks_;
  base::CallbackList<void(const mojom::Geoposition&)> low_accuracy_callbacks_;

  bool user_did_opt_into_location_services_;
  mojom::Geoposition position_;

  // True only in testing, where we want to use a custom position.
  bool ignore_location_updates_;

  // Used to PostTask()s from the geolocation thread to caller thread.
  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // Only to be used on the geolocation thread.
  std::unique_ptr<LocationProvider> arbitrator_;

  mojo::Binding<mojom::GeolocationControl> binding_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationProviderImpl);
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_GEOLOCATION_PROVIDER_IMPL_H_
