// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_ble_transaction.h"

#include <utility>

#include "device/fido/fido_ble_connection.h"
#include "device/fido/fido_constants.h"

namespace device {

FidoBleTransaction::FidoBleTransaction(FidoBleConnection* connection,
                                       uint16_t control_point_length)
    : connection_(connection),
      control_point_length_(control_point_length),
      weak_factory_(this) {
  buffer_.reserve(control_point_length_);
}

FidoBleTransaction::~FidoBleTransaction() = default;

void FidoBleTransaction::WriteRequestFrame(FidoBleFrame request_frame,
                                           FrameCallback callback) {
  DCHECK(!request_frame_ && callback_.is_null());
  request_frame_ = std::move(request_frame);
  callback_ = std::move(callback);

  FidoBleFrameInitializationFragment request_init_fragment;
  std::tie(request_init_fragment, request_cont_fragments_) =
      request_frame_->ToFragments(control_point_length_);
  WriteRequestFragment(request_init_fragment);
}

void FidoBleTransaction::WriteRequestFragment(
    const FidoBleFrameFragment& fragment) {
  buffer_.clear();
  fragment.Serialize(&buffer_);
  // A weak pointer is required, since this call might time out. If that
  // happens, the current FidoBleTransaction could be destroyed.
  connection_->WriteControlPoint(
      buffer_, base::BindOnce(&FidoBleTransaction::OnRequestFragmentWritten,
                              weak_factory_.GetWeakPtr()));
  // WriteRequestFragment() expects an invocation of OnRequestFragmentWritten()
  // soon after.
  StartTimeout();
}

void FidoBleTransaction::OnRequestFragmentWritten(bool success) {
  StopTimeout();
  if (!success) {
    OnError();
    return;
  }

  if (request_cont_fragments_.empty()) {
    // The transaction wrote the full request frame. A response should follow
    // soon after.
    StartTimeout();
    return;
  }

  auto next_request_fragment = std::move(request_cont_fragments_.front());
  request_cont_fragments_.pop();
  WriteRequestFragment(next_request_fragment);
}

void FidoBleTransaction::OnResponseFragment(std::vector<uint8_t> data) {
  StopTimeout();
  if (!response_frame_assembler_) {
    FidoBleFrameInitializationFragment fragment;
    if (!FidoBleFrameInitializationFragment::Parse(data, &fragment)) {
      DLOG(ERROR) << "Malformed Frame Initialization Fragment";
      OnError();
      return;
    }

    response_frame_assembler_.emplace(fragment);
  } else {
    FidoBleFrameContinuationFragment fragment;
    if (!FidoBleFrameContinuationFragment::Parse(data, &fragment)) {
      DLOG(ERROR) << "Malformed Frame Continuation Fragment";
      OnError();
      return;
    }

    response_frame_assembler_->AddFragment(fragment);
  }

  if (!response_frame_assembler_->IsDone()) {
    // Expect the next reponse fragment to arrive soon.
    StartTimeout();
    return;
  }

  FidoBleFrame frame = std::move(*response_frame_assembler_->GetFrame());
  response_frame_assembler_.reset();
  ProcessResponseFrame(std::move(frame));
}

void FidoBleTransaction::ProcessResponseFrame(FidoBleFrame response_frame) {
  DCHECK(request_frame_.has_value());
  if (response_frame.command() == request_frame_->command()) {
    request_frame_.reset();
    std::move(callback_).Run(std::move(response_frame));
    return;
  }

  if (response_frame.command() == FidoBleDeviceCommand::kKeepAlive) {
    DVLOG(2) << "CMD_KEEPALIVE: "
             << static_cast<uint8_t>(response_frame.GetKeepaliveCode());
    // Expect another reponse frame soon.
    StartTimeout();
    return;
  }

  DCHECK_EQ(response_frame.command(), FidoBleDeviceCommand::kError);
  DLOG(ERROR) << "CMD_ERROR: "
              << static_cast<uint8_t>(response_frame.GetErrorCode());
  OnError();
}

void FidoBleTransaction::StartTimeout() {
  timer_.Start(FROM_HERE, kDeviceTimeout, this, &FidoBleTransaction::OnError);
}

void FidoBleTransaction::StopTimeout() {
  timer_.Stop();
}

void FidoBleTransaction::OnError() {
  request_frame_.reset();
  request_cont_fragments_ = base::queue<FidoBleFrameContinuationFragment>();
  response_frame_assembler_.reset();
  std::move(callback_).Run(base::nullopt);
}

}  // namespace device
