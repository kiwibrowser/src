// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_MOCK_TETHER_HOST_RESPONSE_RECORDER_H_
#define CHROMEOS_COMPONENTS_TETHER_MOCK_TETHER_HOST_RESPONSE_RECORDER_H_

#include <vector>

#include "base/macros.h"
#include "chromeos/components/tether/tether_host_response_recorder.h"
#include "components/cryptauth/remote_device_ref.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

namespace tether {

// Test double for TetherHostResponseRecorder.
class MockTetherHostResponseRecorder : public TetherHostResponseRecorder {
 public:
  MockTetherHostResponseRecorder();
  ~MockTetherHostResponseRecorder() override;

  MOCK_METHOD1(RecordSuccessfulTetherAvailabilityResponse,
               void(cryptauth::RemoteDeviceRef));
  MOCK_METHOD1(RecordSuccessfulConnectTetheringResponse,
               void(cryptauth::RemoteDeviceRef));
  MOCK_CONST_METHOD0(GetPreviouslyAvailableHostIds, std::vector<std::string>());
  MOCK_CONST_METHOD0(GetPreviouslyConnectedHostIds, std::vector<std::string>());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTetherHostResponseRecorder);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_MOCK_TETHER_HOST_RESPONSE_RECORDER_H_
