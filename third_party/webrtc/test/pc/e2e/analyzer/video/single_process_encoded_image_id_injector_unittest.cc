/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "test/pc/e2e/analyzer/video/single_process_encoded_image_id_injector.h"

#include <utility>

#include "api/video/encoded_image.h"
#include "rtc_base/buffer.h"
#include "test/gtest.h"

namespace webrtc {
namespace test {
namespace {

rtc::Buffer CreateBufferOfSizeNFilledWithValuesFromX(size_t n, uint8_t x) {
  rtc::Buffer buffer(n);
  for (size_t i = 0; i < n; ++i) {
    buffer[i] = static_cast<uint8_t>(x + i);
  }
  return buffer;
}

}  // namespace

TEST(SingleProcessEncodedImageIdInjector, InjectExtract) {
  SingleProcessEncodedImageIdInjector injector;

  rtc::Buffer buffer = CreateBufferOfSizeNFilledWithValuesFromX(10, 1);

  EncodedImage source(buffer.data(), 10, 10);
  source.SetTimestamp(123456789);

  EncodedImageWithId out =
      injector.ExtractId(injector.InjectId(512, source, 1), 2);
  ASSERT_EQ(out.id, 512);
  ASSERT_EQ(out.image.size(), 10ul);
  ASSERT_EQ(out.image.capacity(), 10ul);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(out.image.data()[i], i + 1);
  }
}

TEST(SingleProcessEncodedImageIdInjector, Inject3Extract3) {
  SingleProcessEncodedImageIdInjector injector;

  rtc::Buffer buffer1 = CreateBufferOfSizeNFilledWithValuesFromX(10, 1);
  rtc::Buffer buffer2 = CreateBufferOfSizeNFilledWithValuesFromX(10, 11);
  rtc::Buffer buffer3 = CreateBufferOfSizeNFilledWithValuesFromX(10, 21);

  // 1st frame
  EncodedImage source1(buffer1.data(), 10, 10);
  source1.SetTimestamp(123456710);
  // 2nd frame 1st spatial layer
  EncodedImage source2(buffer2.data(), 10, 10);
  source2.SetTimestamp(123456720);
  // 2nd frame 2nd spatial layer
  EncodedImage source3(buffer3.data(), 10, 10);
  source3.SetTimestamp(123456720);

  EncodedImage intermediate1 = injector.InjectId(510, source1, 1);
  EncodedImage intermediate2 = injector.InjectId(520, source2, 1);
  EncodedImage intermediate3 = injector.InjectId(520, source3, 1);

  // Extract ids in different order.
  EncodedImageWithId out3 = injector.ExtractId(intermediate3, 2);
  EncodedImageWithId out1 = injector.ExtractId(intermediate1, 2);
  EncodedImageWithId out2 = injector.ExtractId(intermediate2, 2);

  ASSERT_EQ(out1.id, 510);
  ASSERT_EQ(out1.image.size(), 10ul);
  ASSERT_EQ(out1.image.capacity(), 10ul);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(out1.image.data()[i], i + 1);
  }
  ASSERT_EQ(out2.id, 520);
  ASSERT_EQ(out2.image.size(), 10ul);
  ASSERT_EQ(out2.image.capacity(), 10ul);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(out2.image.data()[i], i + 11);
  }
  ASSERT_EQ(out3.id, 520);
  ASSERT_EQ(out3.image.size(), 10ul);
  ASSERT_EQ(out3.image.capacity(), 10ul);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(out3.image.data()[i], i + 21);
  }
}

TEST(SingleProcessEncodedImageIdInjector, InjectExtractFromConcatenated) {
  SingleProcessEncodedImageIdInjector injector;

  rtc::Buffer buffer1 = CreateBufferOfSizeNFilledWithValuesFromX(10, 1);
  rtc::Buffer buffer2 = CreateBufferOfSizeNFilledWithValuesFromX(10, 11);
  rtc::Buffer buffer3 = CreateBufferOfSizeNFilledWithValuesFromX(10, 21);

  EncodedImage source1(buffer1.data(), 10, 10);
  source1.SetTimestamp(123456710);
  EncodedImage source2(buffer2.data(), 10, 10);
  source2.SetTimestamp(123456710);
  EncodedImage source3(buffer3.data(), 10, 10);
  source3.SetTimestamp(123456710);

  // Inject id into 3 images with same frame id.
  EncodedImage intermediate1 = injector.InjectId(512, source1, 1);
  EncodedImage intermediate2 = injector.InjectId(512, source2, 1);
  EncodedImage intermediate3 = injector.InjectId(512, source3, 1);

  // Concatenate them into single encoded image, like it can be done in jitter
  // buffer.
  size_t concatenated_length =
      intermediate1.size() + intermediate2.size() + intermediate3.size();
  rtc::Buffer concatenated_buffer;
  concatenated_buffer.AppendData(intermediate1.data(), intermediate1.size());
  concatenated_buffer.AppendData(intermediate2.data(), intermediate2.size());
  concatenated_buffer.AppendData(intermediate3.data(), intermediate3.size());
  EncodedImage concatenated(concatenated_buffer.data(), concatenated_length,
                            concatenated_length);

  // Extract frame id from concatenated image
  EncodedImageWithId out = injector.ExtractId(concatenated, 2);

  ASSERT_EQ(out.id, 512);
  ASSERT_EQ(out.image.size(), 3 * 10ul);
  ASSERT_EQ(out.image.capacity(), 3 * 10ul);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(out.image.data()[i], i + 1);
    ASSERT_EQ(out.image.data()[i + 10], i + 11);
    ASSERT_EQ(out.image.data()[i + 20], i + 21);
  }
}

}  // namespace test
}  // namespace webrtc
