// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/testing/mock_web_crypto.h"

#include <cstring>
#include <memory>
#include <string>
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

using testing::_;
using testing::DoAll;
using testing::InSequence;
using testing::Return;
using testing::SetArgReferee;

// MemEq(p, len) expects memcmp(arg, p, len) == 0, where |arg| is the argument
// to be matched.
MATCHER_P2(MemEq,
           p,
           len,
           std::string("pointing to memory") + (negation ? " not" : "") +
               " equal to \"" + std::string(static_cast<const char*>(p), len) +
               "\" (length=" + testing::PrintToString(len) + ")") {
  return memcmp(arg, p, len) == 0;
}

void MockWebCryptoDigestor::ExpectConsumeAndFinish(const void* input_data,
                                                   unsigned input_length,
                                                   void* output_data,
                                                   unsigned output_length) {
  InSequence s;

  // Consume should be called with a memory region equal to |input_data|.
  EXPECT_CALL(*this, Consume(MemEq(input_data, input_length), input_length))
      .WillOnce(Return(true));

  // Finish(unsigned char*& result_data, unsigned& result_data_size) {
  //   result_data = output_data;
  //   result_data_size = output_length;
  //   return true;
  // }
  EXPECT_CALL(*this, Finish(_, _))
      .WillOnce(
          DoAll(SetArgReferee<0>(static_cast<unsigned char*>(output_data)),
                SetArgReferee<1>(output_length), Return(true)));
}

MockWebCryptoDigestorFactory::MockWebCryptoDigestorFactory(
    const void* input_data,
    unsigned input_length,
    void* output_data,
    unsigned output_length)
    : input_data_(input_data),
      input_length_(input_length),
      output_data_(output_data),
      output_length_(output_length) {}

MockWebCryptoDigestor* MockWebCryptoDigestorFactory::Create() {
  std::unique_ptr<MockWebCryptoDigestor> digestor(
      MockWebCryptoDigestor::Create());
  digestor->ExpectConsumeAndFinish(input_data_, input_length_, output_data_,
                                   output_length_);
  return digestor.release();
}

}  // namespace blink
