// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_SYSTEM_CLOCK_CLIENT_H_
#define CHROMEOS_DBUS_SYSTEM_CLOCK_CLIENT_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "dbus/object_proxy.h"

namespace chromeos {

// SystemClockClient is used to communicate with the system clock.
class CHROMEOS_EXPORT SystemClockClient : public DBusClient {
 public:
  using GetLastSyncInfoCallback = base::OnceCallback<void(bool synchronized)>;

  // Interface for observing changes from the system clock.
  class Observer {
   public:
    // Called when the status is updated.
    virtual void SystemClockUpdated() {}

    // Called when the system clock has become settable or unsettable, e.g.
    // when the clock syncs with or goes out of sync with the network.
    virtual void SystemClockCanSetTimeChanged(bool can_set_time) {}

   protected:
    virtual ~Observer() {}
  };

  // Adds the given observer.
  virtual void AddObserver(Observer* observer) = 0;
  // Removes the given observer if this object has the observer.
  virtual void RemoveObserver(Observer* observer) = 0;
  // Returns true if this object has the given observer.
  virtual bool HasObserver(const Observer* observer) const = 0;

  // Sets the system clock.
  virtual void SetTime(int64_t time_in_seconds) = 0;

  // Checks if the system time can be set.
  virtual bool CanSetTime() = 0;

  // Runs |callback| asynchronously with the system time's current
  // synchronization state with network time.
  virtual void GetLastSyncInfo(GetLastSyncInfoCallback callback) = 0;

  // Runs the callback as soon as the service becomes available.
  virtual void WaitForServiceToBeAvailable(
      dbus::ObjectProxy::WaitForServiceToBeAvailableCallback callback) = 0;

  // Creates the instance.
  static SystemClockClient* Create();

 protected:
  // Create() should be used instead.
  SystemClockClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemClockClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_SYSTEM_CLOCK_CLIENT_H_
