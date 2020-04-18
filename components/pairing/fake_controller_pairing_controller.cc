// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/pairing/fake_controller_pairing_controller.h"

#include <map>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"

namespace pairing_chromeos {

FakeControllerPairingController::FakeControllerPairingController(
    const std::string& config)
    : current_stage_(STAGE_NONE),
      should_fail_on_connecting_(false),
      connection_lost_begin_(STAGE_NONE),
      connection_lost_end_(STAGE_NONE),
      enrollment_should_fail_(false) {
  ApplyConfig(config);
  AddObserver(this);
}

FakeControllerPairingController::~FakeControllerPairingController() {
  RemoveObserver(this);
}

void FakeControllerPairingController::ApplyConfig(const std::string& config) {
  typedef std::vector<std::string> Tokens;

  base::StringPairs kv_pairs;
  CHECK(base::SplitStringIntoKeyValuePairs(config, ':', ',', &kv_pairs))
      << "Wrong config format.";
  std::map<std::string, std::string> dict(kv_pairs.begin(), kv_pairs.end());

  if (dict.count("async_duration")) {
    int ms = 0;
    CHECK(base::StringToInt(dict["async_duration"], &ms))
        << "Wrong 'async_duration' format.";
    async_duration_ = base::TimeDelta::FromMilliseconds(ms);
  } else {
    async_duration_ = base::TimeDelta::FromMilliseconds(3000);
  }

  should_fail_on_connecting_ =
      dict.count("fail_connecting") && (dict["fail_connecting"] == "1");

  enrollment_should_fail_ =
      dict.count("fail_enrollment") && (dict["fail_enrollment"] == "1");

  if (dict.count("connection_lost")) {
    Tokens lost_begin_end = base::SplitString(
        dict["connection_lost"], "-", base::KEEP_WHITESPACE,
        base::SPLIT_WANT_NONEMPTY);
    CHECK_EQ(2u, lost_begin_end.size()) << "Wrong 'connection_lost' format.";
    int begin = 0;
    int end = 0;
    CHECK(base::StringToInt(lost_begin_end[0], &begin) &&
          base::StringToInt(lost_begin_end[1], &end))
        << "Wrong 'connection_lost' format.";
    CHECK((begin == 0 && end == 0) ||
          (STAGE_WAITING_FOR_CODE_CONFIRMATION <= begin && begin <= end &&
           end <= STAGE_HOST_ENROLLMENT_ERROR))
        << "Wrong 'connection_lost' interval.";
    connection_lost_begin_ = static_cast<Stage>(begin);
    connection_lost_end_ = static_cast<Stage>(end);
  } else {
    connection_lost_begin_ = connection_lost_end_ = STAGE_NONE;
  }

  if (!dict.count("discovery")) {
    dict["discovery"] =
        "F-Device_1~F-Device_5~F-Device_3~L-Device_3~L-Device_1~F-Device_1";
  }
  base::StringPairs events;
  CHECK(
      base::SplitStringIntoKeyValuePairs(dict["discovery"], '-', '~', &events))
      << "Wrong 'discovery' format.";
  DiscoveryScenario scenario;
  for (const auto& event : events) {
    const std::string& type = event.first;
    const std::string& device_id = event.second;
    CHECK(type == "F" || type == "L" || type == "N")
        << "Wrong discovery event type.";
    CHECK(!device_id.empty() || type == "N") << "Empty device ID.";
    scenario.push_back(DiscoveryEvent(
        type == "F" ? DEVICE_FOUND : type == "L" ? DEVICE_LOST : NOTHING_FOUND,
        device_id));
  }
  SetDiscoveryScenario(scenario);

  preset_confirmation_code_ = dict["code"];
  CHECK(preset_confirmation_code_.empty() ||
        (preset_confirmation_code_.length() == 6 &&
         preset_confirmation_code_.find_first_not_of("0123456789") ==
             std::string::npos))
      << "Wrong 'code' format.";
}

void FakeControllerPairingController::SetShouldFailOnConnecting() {
  should_fail_on_connecting_ = true;
}

void FakeControllerPairingController::SetShouldLoseConnection(Stage stage_begin,
                                                              Stage stage_end) {
  connection_lost_begin_ = stage_begin;
  connection_lost_end_ = stage_end;
}

void FakeControllerPairingController::SetEnrollmentShouldFail() {
  enrollment_should_fail_ = true;
}

void FakeControllerPairingController::SetDiscoveryScenario(
    const DiscoveryScenario& discovery_scenario) {
  discovery_scenario_ = discovery_scenario;
  // Check that scenario is valid.
  std::set<std::string> devices;
  for (DiscoveryScenario::const_iterator event = discovery_scenario_.begin();
       event != discovery_scenario_.end();
       ++event) {
    switch (event->first) {
      case DEVICE_FOUND: {
        devices.insert(event->second);
        break;
      }
      case DEVICE_LOST: {
        CHECK(devices.count(event->second));
        devices.erase(event->second);
        break;
      }
      case NOTHING_FOUND: {
        CHECK(++event == discovery_scenario_.end());
        return;
      }
    }
  }
}

void FakeControllerPairingController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FakeControllerPairingController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

ControllerPairingController::Stage
FakeControllerPairingController::GetCurrentStage() {
  return current_stage_;
}

void FakeControllerPairingController::StartPairing() {
  CHECK(current_stage_ == STAGE_NONE);
  ChangeStage(STAGE_DEVICES_DISCOVERY);
}

ControllerPairingController::DeviceIdList
FakeControllerPairingController::GetDiscoveredDevices() {
  CHECK(current_stage_ == STAGE_DEVICES_DISCOVERY);
  return DeviceIdList(discovered_devices_.begin(), discovered_devices_.end());
}

void FakeControllerPairingController::ChooseDeviceForPairing(
    const std::string& device_id) {
  CHECK(current_stage_ == STAGE_DEVICES_DISCOVERY);
  CHECK(discovered_devices_.count(device_id));
  choosen_device_ = device_id;
  ChangeStage(STAGE_ESTABLISHING_CONNECTION);
}

void FakeControllerPairingController::RepeatDiscovery() {
  CHECK(current_stage_ == STAGE_DEVICE_NOT_FOUND ||
        current_stage_ == STAGE_ESTABLISHING_CONNECTION_ERROR ||
        current_stage_ == STAGE_HOST_ENROLLMENT_ERROR);
  ChangeStage(STAGE_DEVICES_DISCOVERY);
}

std::string FakeControllerPairingController::GetConfirmationCode() {
  CHECK(current_stage_ == STAGE_WAITING_FOR_CODE_CONFIRMATION);
  if (confirmation_code_.empty()) {
    if (preset_confirmation_code_.empty()) {
      for (int i = 0; i < 6; ++i)
        confirmation_code_.push_back(base::RandInt('0', '9'));
    } else {
      confirmation_code_ = preset_confirmation_code_;
    }
  }
  return confirmation_code_;
}

void FakeControllerPairingController::SetConfirmationCodeIsCorrect(
    bool correct) {
  CHECK(current_stage_ == STAGE_WAITING_FOR_CODE_CONFIRMATION);
  if (correct)
    ChangeStage(STAGE_HOST_UPDATE_IN_PROGRESS);
  else
    ChangeStage(STAGE_DEVICES_DISCOVERY);
}

void FakeControllerPairingController::SetHostNetwork(
    const std::string& onc_spec) {}

void FakeControllerPairingController::SetHostConfiguration(
    bool accepted_eula,
    const std::string& lang,
    const std::string& timezone,
    bool send_reports,
    const std::string& keyboard_layout) {
}

void FakeControllerPairingController::OnAuthenticationDone(
    const std::string& domain,
    const std::string& auth_token) {
  CHECK(current_stage_ == STAGE_WAITING_FOR_CREDENTIALS);
  ChangeStage(STAGE_HOST_ENROLLMENT_IN_PROGRESS);
}

void FakeControllerPairingController::StartSession() {
  CHECK(current_stage_ == STAGE_HOST_ENROLLMENT_SUCCESS);
  ChangeStage(STAGE_FINISHED);
}

void FakeControllerPairingController::ChangeStage(Stage new_stage) {
  if (current_stage_ == new_stage)
    return;
  current_stage_ = new_stage;
  for (Observer& observer : observers_)
    observer.PairingStageChanged(new_stage);
}

void FakeControllerPairingController::ChangeStageLater(Stage new_stage) {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&FakeControllerPairingController::ChangeStage,
                            base::Unretained(this), new_stage),
      async_duration_);
}

void FakeControllerPairingController::ExecuteDiscoveryEvent(
    size_t event_position) {
  if (current_stage_ != STAGE_DEVICES_DISCOVERY)
    return;
  CHECK(event_position < discovery_scenario_.size());
  const DiscoveryEvent& event = discovery_scenario_[event_position];
  switch (event.first) {
    case DEVICE_FOUND: {
      DeviceFound(event.second);
      break;
    }
    case DEVICE_LOST: {
      DeviceLost(event.second);
      break;
    }
    case NOTHING_FOUND: {
      ChangeStage(STAGE_DEVICE_NOT_FOUND);
      break;
    }
  }
  if (++event_position == discovery_scenario_.size()) {
    return;
  }
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&FakeControllerPairingController::ExecuteDiscoveryEvent,
                 base::Unretained(this), event_position),
      async_duration_);
}

void FakeControllerPairingController::DeviceFound(
    const std::string& device_id) {
  CHECK(current_stage_ == STAGE_DEVICES_DISCOVERY);
  discovered_devices_.insert(device_id);
  for (Observer& observer : observers_)
    observer.DiscoveredDevicesListChanged();
}

void FakeControllerPairingController::DeviceLost(const std::string& device_id) {
  CHECK(current_stage_ == STAGE_DEVICES_DISCOVERY);
  discovered_devices_.erase(device_id);
  for (Observer& observer : observers_)
    observer.DiscoveredDevicesListChanged();
}

void FakeControllerPairingController::PairingStageChanged(Stage new_stage) {
  Stage next_stage = STAGE_NONE;
  switch (new_stage) {
    case STAGE_DEVICES_DISCOVERY: {
      discovered_devices_.clear();
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&FakeControllerPairingController::ExecuteDiscoveryEvent,
                     base::Unretained(this), 0),
          async_duration_);
      break;
    }
    case STAGE_ESTABLISHING_CONNECTION: {
      if (should_fail_on_connecting_) {
        next_stage = STAGE_ESTABLISHING_CONNECTION_ERROR;
        should_fail_on_connecting_ = false;
      } else {
        confirmation_code_.clear();
        next_stage = STAGE_WAITING_FOR_CODE_CONFIRMATION;
      }
      break;
    }
    case STAGE_HOST_UPDATE_IN_PROGRESS: {
      next_stage = STAGE_WAITING_FOR_CREDENTIALS;
      break;
    }
    case STAGE_HOST_ENROLLMENT_IN_PROGRESS: {
      if (enrollment_should_fail_) {
        enrollment_should_fail_ = false;
        next_stage = STAGE_HOST_ENROLLMENT_ERROR;
      } else {
        next_stage = STAGE_HOST_ENROLLMENT_SUCCESS;
      }
      break;
    }
    case STAGE_HOST_CONNECTION_LOST: {
      next_stage = connection_lost_end_;
      connection_lost_end_ = STAGE_NONE;
      break;
    }
    default:
      break;
  }
  if (new_stage == connection_lost_begin_) {
    connection_lost_begin_ = STAGE_NONE;
    next_stage = STAGE_HOST_CONNECTION_LOST;
  }
  if (next_stage != STAGE_NONE)
    ChangeStageLater(next_stage);
}

void FakeControllerPairingController::DiscoveredDevicesListChanged() {
}

}  // namespace pairing_chromeos
