// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/hammerd_client.h"

#include <string>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/hammerd/dbus-constants.h"

namespace chromeos {

namespace {

class HammerdClientImpl : public HammerdClient {
 public:
  HammerdClientImpl() = default;
  ~HammerdClientImpl() override = default;

  // HammerdClient:
  void Init(dbus::Bus* bus) override {
    bus_proxy_ =
        bus->GetObjectProxy(hammerd::kHammerdServiceName,
                            dbus::ObjectPath(hammerd::kHammerdServicePath));

    const struct {
      std::string signal_name;
      dbus::ObjectProxy::SignalCallback signal_handler;
    } kSignals[] = {
        {hammerd::kBaseFirmwareNeedUpdateSignal,
         base::BindRepeating(&HammerdClientImpl::OnBaseFirmwareUpdateNeeded,
                             weak_ptr_factory_.GetWeakPtr())},
        {hammerd::kBaseFirmwareUpdateStartedSignal,
         base::BindRepeating(&HammerdClientImpl::OnBaseFirmwareUpdateStarted,
                             weak_ptr_factory_.GetWeakPtr())},
        {hammerd::kBaseFirmwareUpdateSucceededSignal,
         base::BindRepeating(&HammerdClientImpl::OnBaseFirmwareUpdateSucceeded,
                             weak_ptr_factory_.GetWeakPtr())},
        {hammerd::kBaseFirmwareUpdateFailedSignal,
         base::BindRepeating(&HammerdClientImpl::OnBaseFirmwareUpdateFailed,
                             weak_ptr_factory_.GetWeakPtr())},
        {hammerd::kPairChallengeSucceededSignal,
         base::BindRepeating(&HammerdClientImpl::OnPairChallengeSucceeded,
                             weak_ptr_factory_.GetWeakPtr())},
        {hammerd::kPairChallengeFailedSignal,
         base::BindRepeating(&HammerdClientImpl::OnPairChallengeFailed,
                             weak_ptr_factory_.GetWeakPtr())},
        {hammerd::kInvalidBaseConnectedSignal,
         base::BindRepeating(&HammerdClientImpl::OnInvalidBaseConnected,
                             weak_ptr_factory_.GetWeakPtr())},
    };

    for (const auto& signal : kSignals) {
      bus_proxy_->ConnectToSignal(
          hammerd::kHammerdInterface, signal.signal_name, signal.signal_handler,
          base::BindOnce(&HammerdClientImpl::OnSignalConnect,
                         weak_ptr_factory_.GetWeakPtr()));
    }
  }
  void AddObserver(Observer* observer) override {
    observers_.AddObserver(observer);
  }
  void RemoveObserver(Observer* observer) override {
    observers_.RemoveObserver(observer);
  }

 private:
  void OnSignalConnect(const std::string& interface,
                       const std::string& signal,
                       bool succeeded) {
    LOG_IF(ERROR, !succeeded)
        << "Connect to " << interface << ":" << signal << " failed.";
  }

  void OnBaseFirmwareUpdateNeeded(dbus::Signal* signal) {
    DCHECK_EQ(signal->GetInterface(), hammerd::kHammerdInterface);
    DCHECK_EQ(signal->GetMember(), hammerd::kBaseFirmwareNeedUpdateSignal);

    for (auto& observer : observers_)
      observer.BaseFirmwareUpdateNeeded();
  }

  void OnBaseFirmwareUpdateStarted(dbus::Signal* signal) {
    DCHECK_EQ(signal->GetInterface(), hammerd::kHammerdInterface);
    DCHECK_EQ(signal->GetMember(), hammerd::kBaseFirmwareNeedUpdateSignal);

    for (auto& observer : observers_)
      observer.BaseFirmwareUpdateStarted();
  }

  void OnBaseFirmwareUpdateSucceeded(dbus::Signal* signal) {
    DCHECK_EQ(signal->GetInterface(), hammerd::kHammerdInterface);
    DCHECK_EQ(signal->GetMember(), hammerd::kBaseFirmwareNeedUpdateSignal);

    for (auto& observer : observers_)
      observer.BaseFirmwareUpdateSucceeded();
  }

  void OnBaseFirmwareUpdateFailed(dbus::Signal* signal) {
    DCHECK_EQ(signal->GetInterface(), hammerd::kHammerdInterface);
    DCHECK_EQ(signal->GetMember(), hammerd::kBaseFirmwareNeedUpdateSignal);

    for (auto& observer : observers_)
      observer.BaseFirmwareUpdateFailed();
  }

  void OnPairChallengeSucceeded(dbus::Signal* signal) {
    DCHECK_EQ(signal->GetInterface(), hammerd::kHammerdInterface);
    DCHECK_EQ(signal->GetMember(), hammerd::kBaseFirmwareNeedUpdateSignal);

    dbus::MessageReader reader(signal);

    const uint8_t* data = nullptr;
    size_t length = 0;
    if (!reader.PopArrayOfBytes(&data, &length))
      return;

    for (auto& observer : observers_) {
      observer.PairChallengeSucceeded(
          std::vector<uint8_t>(data, data + length));
    }
  }

  void OnPairChallengeFailed(dbus::Signal* signal) {
    DCHECK_EQ(signal->GetInterface(), hammerd::kHammerdInterface);
    DCHECK_EQ(signal->GetMember(), hammerd::kBaseFirmwareNeedUpdateSignal);

    for (auto& observer : observers_)
      observer.PairChallengeFailed();
  }

  void OnInvalidBaseConnected(dbus::Signal* signal) {
    DCHECK_EQ(signal->GetInterface(), hammerd::kHammerdInterface);
    DCHECK_EQ(signal->GetMember(), hammerd::kBaseFirmwareNeedUpdateSignal);

    for (auto& observer : observers_)
      observer.InvalidBaseConnected();
  }

  dbus::ObjectProxy* bus_proxy_ = nullptr;
  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<HammerdClientImpl> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(HammerdClientImpl);
};

}  // namespace

// static
std::unique_ptr<HammerdClient> HammerdClient::Create() {
  return std::make_unique<HammerdClientImpl>();
}

}  // namespace chromeos
