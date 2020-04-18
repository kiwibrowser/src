// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_BLE_TRANSACTION_H_
#define DEVICE_FIDO_FIDO_BLE_TRANSACTION_H_

#include <memory>
#include <vector>

#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/timer/timer.h"
#include "device/fido/fido_ble_frames.h"

namespace device {

class FidoBleConnection;

// This class encapsulates logic related to a single U2F BLE request and
// response. FidoBleTransaction is owned by FidoBleDevice, which is the only
// class that should make use of this class.
class FidoBleTransaction {
 public:
  using FrameCallback = base::OnceCallback<void(base::Optional<FidoBleFrame>)>;

  FidoBleTransaction(FidoBleConnection* connection,
                     uint16_t control_point_length);
  ~FidoBleTransaction();

  void WriteRequestFrame(FidoBleFrame request_frame, FrameCallback callback);
  void OnResponseFragment(std::vector<uint8_t> data);

 private:
  void WriteRequestFragment(const FidoBleFrameFragment& fragment);
  void OnRequestFragmentWritten(bool success);
  void ProcessResponseFrame(FidoBleFrame response_frame);

  void StartTimeout();
  void StopTimeout();
  void OnError();

  FidoBleConnection* connection_;
  uint16_t control_point_length_;

  base::Optional<FidoBleFrame> request_frame_;
  FrameCallback callback_;

  base::queue<FidoBleFrameContinuationFragment> request_cont_fragments_;
  base::Optional<FidoBleFrameAssembler> response_frame_assembler_;

  std::vector<uint8_t> buffer_;
  base::OneShotTimer timer_;

  base::WeakPtrFactory<FidoBleTransaction> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FidoBleTransaction);
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_BLE_TRANSACTION_H_
