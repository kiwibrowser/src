// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_GLUE_JINGLE_GLUE_MOCK_OBJECTS_H_
#define JINGLE_GLUE_JINGLE_GLUE_MOCK_OBJECTS_H_

#include <stddef.h>

#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/webrtc/rtc_base/stream.h"

namespace jingle_glue {

class MockStream : public rtc::StreamInterface {
 public:
  MockStream();
  ~MockStream() override;

  MOCK_CONST_METHOD0(GetState, rtc::StreamState());

  MOCK_METHOD4(Read, rtc::StreamResult(void*, size_t, size_t*, int*));
  MOCK_METHOD4(Write, rtc::StreamResult(const void*, size_t,
                                              size_t*, int*));
  MOCK_CONST_METHOD1(GetAvailable, bool(size_t*));
  MOCK_METHOD0(Close, void());

  MOCK_METHOD3(PostEvent, void(rtc::Thread*, int, int));
  MOCK_METHOD2(PostEvent, void(int, int));
};

}  // namespace jingle_glue

#endif  // JINGLE_GLUE_JINGLE_GLUE_MOCK_OBJECTS_H_
