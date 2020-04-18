// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_MEDIA_ANALYTICS_CLIENT_H_
#define CHROMEOS_DBUS_MEDIA_ANALYTICS_CLIENT_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "chromeos/media_perception/media_perception.pb.h"

namespace chromeos {

// MediaAnalyticsClient is used to communicate with a media analytics process
// running outside of Chrome.
class CHROMEOS_EXPORT MediaAnalyticsClient : public DBusClient {
 public:
  class Observer {
   public:
    // Called when DetectionSignal is received.
    virtual void OnDetectionSignal(
        const mri::MediaPerception& media_perception) = 0;

   protected:
    virtual ~Observer() = default;
  };

  ~MediaAnalyticsClient() override;

  // Adds or removes an observer.
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Gets the media analytics process state.
  virtual void GetState(DBusMethodCallback<mri::State> callback) = 0;

  // Sets the media analytics process state. |state.status| is expected to be
  // set.
  virtual void SetState(const mri::State& state,
                        DBusMethodCallback<mri::State> callback) = 0;

  // API for getting diagnostic information from the media analytics process
  // over D-Bus as a Diagnostics proto message.
  virtual void GetDiagnostics(
      DBusMethodCallback<mri::Diagnostics> callback) = 0;

  // Factory function, creates new instance and returns ownership.
  // For normal usage, access the singleton via DbusThreadManager::Get().
  static MediaAnalyticsClient* Create();

 protected:
  MediaAnalyticsClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaAnalyticsClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_MEDIA_ANALYTICS_CLIENT_H_
