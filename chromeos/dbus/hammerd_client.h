// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_HAMMERD_CLIENT_H_
#define CHROMEOS_DBUS_HAMMERD_CLIENT_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"

namespace chromeos {

// Client for hammerd service - the service that manages pairing and updates for
// physically connected bases of detachable devices (hammers). The client
// listens for dbus signals related to:
//  * the connected base firmware updating state
//  * the connected base pairing events.
// The client forwards the received signals to its observers (together with any
// data extracted from the signal object).
class CHROMEOS_EXPORT HammerdClient : public DBusClient {
 public:
  class Observer {
   public:
    virtual ~Observer() {}

    // Base firmware requires an update.
    virtual void BaseFirmwareUpdateNeeded() = 0;

    // Base firmware has started updating - BaseFirmwareUpdateCompleted() is
    // expected to be dispatched when the update completes.
    virtual void BaseFirmwareUpdateStarted() = 0;

    // Base firmware update has completed successfully.
    virtual void BaseFirmwareUpdateSucceeded() = 0;

    // Base firmware update has failed.
    virtual void BaseFirmwareUpdateFailed() = 0;

    // A base has been attached, and it was successfully authenticated.
    // |base_id| - identifies the authenticated base.
    virtual void PairChallengeSucceeded(
        const std::vector<uint8_t>& base_id) = 0;

    // A base has been attached, but was not successfully authenticated.
    virtual void PairChallengeFailed() = 0;

    // An invalid base has been connected.
    virtual void InvalidBaseConnected() = 0;
  };

  ~HammerdClient() override = default;

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  static std::unique_ptr<HammerdClient> Create();
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_HAMMERD_CLIENT_H_
