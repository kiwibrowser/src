// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NIGHT_LIGHT_NIGHT_LIGHT_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_NIGHT_LIGHT_NIGHT_LIGHT_CLIENT_H_

#include <memory>

#include "ash/public/interfaces/night_light_controller.mojom.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "chromeos/geolocation/simple_geolocation_provider.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace base {
class Clock;
}  // namespace base

namespace net {
class URLRequestContextGetter;
}

// Periodically requests the IP-based geolocation and provides it to the
// NightLightController running in ash.
class NightLightClient : public ash::mojom::NightLightClient {
 public:
  explicit NightLightClient(net::URLRequestContextGetter* url_context_getter);
  ~NightLightClient() override;

  // Starts watching changes in the Night Light schedule type in order to begin
  // periodically pushing user's IP-based geoposition to NightLightController as
  // long as the type is set to "sunset to sunrise".
  void Start();

  // ash::mojom::NightLightClient:
  void OnScheduleTypeChanged(
      ash::mojom::NightLightController::ScheduleType new_type) override;

  const base::OneShotTimer& timer() const { return *timer_; }

  base::Time last_successful_geo_request_time() const {
    return last_successful_geo_request_time_;
  }

  bool using_geoposition() const { return using_geoposition_; }

  static base::TimeDelta GetNextRequestDelayAfterSuccessForTesting();

  void SetNightLightControllerPtrForTesting(
      ash::mojom::NightLightControllerPtr controller);

  void FlushNightLightControllerForTesting();

  void SetTimerForTesting(std::unique_ptr<base::OneShotTimer> timer);

  void SetClockForTesting(base::Clock* clock);

 protected:
  void OnGeoposition(const chromeos::Geoposition& position,
                     bool server_error,
                     const base::TimeDelta elapsed);

 private:
  base::Time GetNow() const;

  // Sends the most recent valid geoposition to NightLightController in ash.
  void SendCurrentGeoposition();

  void ScheduleNextRequest(base::TimeDelta delay);

  // Virtual so that it can be overriden by a fake implementation in unit tests
  // that doesn't request actual geopositions.
  virtual void RequestGeoposition();

  // The IP-based geolocation provider.
  chromeos::SimpleGeolocationProvider provider_;

  ash::mojom::NightLightControllerPtr night_light_controller_;
  mojo::Binding<ash::mojom::NightLightClient> binding_;

  // Delay after which a new request is retried after a failed one.
  base::TimeDelta backoff_delay_;

  std::unique_ptr<base::OneShotTimer> timer_;

  // Optional Used in tests to override the time of "Now".
  base::Clock* clock_ = nullptr;  // Not owned.

  // Last successful geoposition coordinates and its timestamp.
  base::Time last_successful_geo_request_time_;
  double latitude_ = 0.0;
  double longitude_ = 0.0;

  // True as long as the schedule type is set to "sunset to sunrise", which
  // means this client will be retrieving the IP-based geoposition once per day.
  bool using_geoposition_ = false;

  DISALLOW_COPY_AND_ASSIGN(NightLightClient);
};

#endif  // CHROME_BROWSER_CHROMEOS_NIGHT_LIGHT_NIGHT_LIGHT_CLIENT_H_
