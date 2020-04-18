// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/pairing/fake_host_pairing_controller.h"

#include <stddef.h>

#include <map>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/threading/thread_task_runner_handle.h"

namespace {

const int kDefaultAsyncDurationMs = 3000;
const size_t kCodeLength = 6;

}  // namespace

namespace pairing_chromeos {

FakeHostPairingController::FakeHostPairingController(const std::string& config)
    : current_stage_(STAGE_NONE),
      enrollment_should_fail_(false),
      start_after_update_(false) {
  ApplyConfig(config);
  AddObserver(this);
}

FakeHostPairingController::~FakeHostPairingController() {
  RemoveObserver(this);
}

void FakeHostPairingController::ApplyConfig(const std::string& config) {
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
    async_duration_ =
      base::TimeDelta::FromMilliseconds(kDefaultAsyncDurationMs);
  }

  start_after_update_ = dict["start_after_update"] == "1";

  enrollment_should_fail_ = dict["fail_enrollment"] == "1";

  if (dict.count("code")) {
    confirmation_code_ = dict["code"];
  } else {
    confirmation_code_.clear();
    for (size_t i = 0; i < kCodeLength; ++i)
      confirmation_code_.push_back(base::RandInt('0', '9'));
  }
  CHECK(confirmation_code_.length() == kCodeLength &&
        confirmation_code_.find_first_not_of("0123456789") == std::string::npos)
      << "Wrong 'code' format.";

  device_name_ =
      dict.count("device_name") ? dict["device_name"] : "Chromebox-01";

  enrollment_domain_ = dict.count("domain") ? dict["domain"] : "example.com";
}

void FakeHostPairingController::ChangeStage(Stage new_stage) {
  if (current_stage_ == new_stage)
    return;
  current_stage_ = new_stage;
  for (Observer& observer : observers_)
    observer.PairingStageChanged(new_stage);
}

void FakeHostPairingController::ChangeStageLater(Stage new_stage) {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&FakeHostPairingController::ChangeStage,
                            base::Unretained(this), new_stage),
      async_duration_);
}

void FakeHostPairingController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FakeHostPairingController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

HostPairingController::Stage FakeHostPairingController::GetCurrentStage() {
  return current_stage_;
}

void FakeHostPairingController::StartPairing() {
  CHECK(current_stage_ == STAGE_NONE);
  if (start_after_update_) {
    ChangeStage(STAGE_WAITING_FOR_CONTROLLER_AFTER_UPDATE);
  } else {
    ChangeStage(STAGE_WAITING_FOR_CONTROLLER);
  }
}

std::string FakeHostPairingController::GetDeviceName() {
  return device_name_;
}

std::string FakeHostPairingController::GetConfirmationCode() {
  CHECK(current_stage_ == STAGE_WAITING_FOR_CODE_CONFIRMATION);
  return confirmation_code_;
}

std::string FakeHostPairingController::GetEnrollmentDomain() {
  return enrollment_domain_;
}

void FakeHostPairingController::OnNetworkConnectivityChanged(
    Connectivity connectivity_status) {
}

void FakeHostPairingController::OnUpdateStatusChanged(
    UpdateStatus update_status) {
}

void FakeHostPairingController::OnEnrollmentStatusChanged(
    EnrollmentStatus enrollment_status) {
}

void FakeHostPairingController::SetPermanentId(
    const std::string& permanent_id) {
}

void FakeHostPairingController::SetErrorCodeAndMessage(
    int error_code,
    const std::string& error_message) {}

void FakeHostPairingController::Reset() {
}

void FakeHostPairingController::PairingStageChanged(Stage new_stage) {
  switch (new_stage) {
    case STAGE_WAITING_FOR_CONTROLLER: {
      ChangeStageLater(STAGE_WAITING_FOR_CODE_CONFIRMATION);
      break;
    }
    case STAGE_WAITING_FOR_CODE_CONFIRMATION: {
      ChangeStageLater(STAGE_WAITING_FOR_CONTROLLER_AFTER_UPDATE);
      break;
    }
    case STAGE_WAITING_FOR_CONTROLLER_AFTER_UPDATE: {
      ChangeStageLater(STAGE_WAITING_FOR_CREDENTIALS);
      break;
    }
    case STAGE_WAITING_FOR_CREDENTIALS: {
      ChangeStageLater(STAGE_ENROLLING);
      break;
    }
    case STAGE_ENROLLING: {
      if (enrollment_should_fail_) {
        enrollment_should_fail_ = false;
        ChangeStageLater(STAGE_ENROLLMENT_ERROR);
      } else {
        ChangeStageLater(STAGE_ENROLLMENT_SUCCESS);
      }
      break;
    }
    case STAGE_ENROLLMENT_ERROR: {
      ChangeStageLater(STAGE_WAITING_FOR_CONTROLLER_AFTER_UPDATE);
      break;
    }
    case STAGE_ENROLLMENT_SUCCESS: {
      ChangeStageLater(STAGE_FINISHED);
      break;
    }
    default: { break; }
  }
}

}  // namespace pairing_chromeos
