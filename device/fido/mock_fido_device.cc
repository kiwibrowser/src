// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/mock_fido_device.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/apdu/apdu_response.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/fido_test_data.h"

namespace device {

// Matcher to compare the fist byte of the incoming requests.
MATCHER_P(IsCtap2Command, expected_command, "") {
  return !arg.empty() && arg[0] == base::strict_cast<uint8_t>(expected_command);
}

MockFidoDevice::MockFidoDevice() : weak_factory_(this) {}
MockFidoDevice::~MockFidoDevice() = default;

void MockFidoDevice::TryWink(WinkCallback cb) {
  TryWinkRef(cb);
}

void MockFidoDevice::DeviceTransact(std::vector<uint8_t> command,
                                    DeviceCallback cb) {
  DeviceTransactPtr(command, cb);
}

// static
void MockFidoDevice::NotSatisfied(const std::vector<uint8_t>& command,
                                  DeviceCallback& cb) {
  std::move(cb).Run(apdu::ApduResponse(
                        std::vector<uint8_t>(),
                        apdu::ApduResponse::Status::SW_CONDITIONS_NOT_SATISFIED)
                        .GetEncodedResponse());
}

// static
void MockFidoDevice::WrongData(const std::vector<uint8_t>& command,
                               DeviceCallback& cb) {
  std::move(cb).Run(
      apdu::ApduResponse(std::vector<uint8_t>(),
                         apdu::ApduResponse::Status::SW_WRONG_DATA)
          .GetEncodedResponse());
}

// static
void MockFidoDevice::NoErrorSign(const std::vector<uint8_t>& command,
                                 DeviceCallback& cb) {
  std::move(cb).Run(
      apdu::ApduResponse(
          std::vector<uint8_t>(std::begin(test_data::kTestU2fSignResponse),
                               std::end(test_data::kTestU2fSignResponse)),
          apdu::ApduResponse::Status::SW_NO_ERROR)
          .GetEncodedResponse());
}

// static
void MockFidoDevice::NoErrorRegister(const std::vector<uint8_t>& command,
                                     DeviceCallback& cb) {
  std::move(cb).Run(
      apdu::ApduResponse(
          std::vector<uint8_t>(std::begin(test_data::kTestU2fRegisterResponse),
                               std::end(test_data::kTestU2fRegisterResponse)),
          apdu::ApduResponse::Status::SW_NO_ERROR)
          .GetEncodedResponse());
}

// static
void MockFidoDevice::SignWithCorruptedResponse(
    const std::vector<uint8_t>& command,
    DeviceCallback& cb) {
  std::move(cb).Run(
      apdu::ApduResponse(
          std::vector<uint8_t>(
              std::begin(test_data::kTestCorruptedU2fSignResponse),
              std::end(test_data::kTestCorruptedU2fSignResponse)),
          apdu::ApduResponse::Status::SW_NO_ERROR)
          .GetEncodedResponse());
}

// static
void MockFidoDevice::WinkDoNothing(WinkCallback& cb) {
  std::move(cb).Run();
}

void MockFidoDevice::ExpectWinkedAtLeastOnce() {
  EXPECT_CALL(*this, TryWinkRef(::testing::_)).Times(::testing::AtLeast(1));
}

void MockFidoDevice::ExpectCtap2CommandAndRespondWith(
    CtapRequestCommand command,
    base::Optional<base::span<const uint8_t>> response,
    base::TimeDelta delay) {
  auto data = fido_parsing_utils::MaterializeOrNull(response);
  auto send_response = [ data(std::move(data)), delay ](DeviceCallback & cb) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::BindOnce(std::move(cb), std::move(data)), delay);
  };

  EXPECT_CALL(*this, DeviceTransactPtr(IsCtap2Command(command), ::testing::_))
      .WillOnce(::testing::WithArg<1>(::testing::Invoke(send_response)));
}

void MockFidoDevice::ExpectRequestAndRespondWith(
    base::span<const uint8_t> request,
    base::Optional<base::span<const uint8_t>> response,
    base::TimeDelta delay) {
  auto data = fido_parsing_utils::MaterializeOrNull(response);
  auto send_response = [ data(std::move(data)), delay ](DeviceCallback & cb) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::BindOnce(std::move(cb), std::move(data)), delay);
  };

  auto request_as_vector = fido_parsing_utils::Materialize(request);
  EXPECT_CALL(*this,
              DeviceTransactPtr(std::move(request_as_vector), ::testing::_))
      .WillOnce(::testing::WithArg<1>(::testing::Invoke(send_response)));
}

void MockFidoDevice::ExpectCtap2CommandAndDoNotRespond(
    CtapRequestCommand command) {
  EXPECT_CALL(*this, DeviceTransactPtr(IsCtap2Command(command), ::testing::_));
}

void MockFidoDevice::ExpectRequestAndDoNotRespond(
    base::span<const uint8_t> request) {
  auto request_as_vector = fido_parsing_utils::Materialize(request);
  EXPECT_CALL(*this,
              DeviceTransactPtr(std::move(request_as_vector), ::testing::_));
}

base::WeakPtr<FidoDevice> MockFidoDevice::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace device
