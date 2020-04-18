// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_HAMMERD_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_HAMMERD_CLIENT_H_

#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/hammerd_client.h"

namespace chromeos {

class CHROMEOS_EXPORT FakeHammerdClient : public HammerdClient {
 public:
  FakeHammerdClient();
  ~FakeHammerdClient() override;

  // HammerdClient:
  void Init(dbus::Bus* bus) override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

  // Methods to simulate signals exposed by the hammerd service API.
  void FireBaseFirmwareNeedUpdateSignal();
  void FireBaseFirmwareUpdateStartedSignal();
  void FireBaseFirmwareUpdateSucceededSignal();
  void FireBaseFirmwareUpdateFailedSignal();
  void FirePairChallengeSucceededSignal(const std::vector<uint8_t>& base_id);
  void FirePairChallengeFailedSignal();
  void FireInvalidBaseConnectedSignal();

 private:
  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(FakeHammerdClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_HAMMERD_CLIENT_H_
