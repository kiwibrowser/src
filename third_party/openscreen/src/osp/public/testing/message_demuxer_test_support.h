// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_TESTING_MESSAGE_DEMUXER_TEST_SUPPORT_H_
#define OSP_PUBLIC_TESTING_MESSAGE_DEMUXER_TEST_SUPPORT_H_

#include "osp/public/message_demuxer.h"
#include "platform/api/time.h"
#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"

namespace openscreen {

class MockMessageCallback final : public MessageDemuxer::MessageCallback {
 public:
  ~MockMessageCallback() override = default;

  MOCK_METHOD6(OnStreamMessage,
               ErrorOr<size_t>(uint64_t endpoint_id,
                               uint64_t connection_id,
                               msgs::Type message_type,
                               const uint8_t* buffer,
                               size_t buffer_size,
                               platform::Clock::time_point now));
};

}  // namespace openscreen

#endif  // OSP_PUBLIC_TESTING_MESSAGE_DEMUXER_TEST_SUPPORT_H_
