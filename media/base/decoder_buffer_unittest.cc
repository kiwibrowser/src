// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/decoder_buffer.h"

#include <stdint.h>
#include <string.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/shared_memory.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

TEST(DecoderBufferTest, Constructors) {
  scoped_refptr<DecoderBuffer> buffer(new DecoderBuffer(0));
  EXPECT_TRUE(buffer->data());
  EXPECT_EQ(0u, buffer->data_size());
  EXPECT_FALSE(buffer->end_of_stream());
  EXPECT_FALSE(buffer->is_key_frame());

  const size_t kTestSize = 10;
  scoped_refptr<DecoderBuffer> buffer3(new DecoderBuffer(kTestSize));
  ASSERT_TRUE(buffer3.get());
  EXPECT_EQ(kTestSize, buffer3->data_size());
}

TEST(DecoderBufferTest, CreateEOSBuffer) {
  scoped_refptr<DecoderBuffer> buffer(DecoderBuffer::CreateEOSBuffer());
  EXPECT_TRUE(buffer->end_of_stream());
}

TEST(DecoderBufferTest, CopyFrom) {
  const uint8_t kData[] = "hello";
  const size_t kDataSize = arraysize(kData);

  scoped_refptr<DecoderBuffer> buffer2(DecoderBuffer::CopyFrom(
      reinterpret_cast<const uint8_t*>(&kData), kDataSize));
  ASSERT_TRUE(buffer2.get());
  EXPECT_NE(kData, buffer2->data());
  EXPECT_EQ(buffer2->data_size(), kDataSize);
  EXPECT_EQ(0, memcmp(buffer2->data(), kData, kDataSize));
  EXPECT_FALSE(buffer2->end_of_stream());
  EXPECT_FALSE(buffer2->is_key_frame());

  scoped_refptr<DecoderBuffer> buffer3(DecoderBuffer::CopyFrom(
      reinterpret_cast<const uint8_t*>(&kData), kDataSize,
      reinterpret_cast<const uint8_t*>(&kData), kDataSize));
  ASSERT_TRUE(buffer3.get());
  EXPECT_NE(kData, buffer3->data());
  EXPECT_EQ(buffer3->data_size(), kDataSize);
  EXPECT_EQ(0, memcmp(buffer3->data(), kData, kDataSize));
  EXPECT_NE(kData, buffer3->side_data());
  EXPECT_EQ(buffer3->side_data_size(), kDataSize);
  EXPECT_EQ(0, memcmp(buffer3->side_data(), kData, kDataSize));
  EXPECT_FALSE(buffer3->end_of_stream());
  EXPECT_FALSE(buffer3->is_key_frame());
}

TEST(DecoderBufferTest, FromSharedMemoryHandle) {
  const uint8_t kData[] = "hello";
  const size_t kDataSize = arraysize(kData);

  base::SharedMemory mem;
  ASSERT_TRUE(mem.CreateAndMapAnonymous(kDataSize));
  memcpy(mem.memory(), kData, kDataSize);

  scoped_refptr<DecoderBuffer> buffer(
      DecoderBuffer::FromSharedMemoryHandle(mem.TakeHandle(), 0, kDataSize));
  ASSERT_TRUE(buffer.get());
  EXPECT_EQ(buffer->data_size(), kDataSize);
  EXPECT_EQ(0, memcmp(buffer->data(), kData, kDataSize));
  EXPECT_FALSE(buffer->end_of_stream());
  EXPECT_FALSE(buffer->is_key_frame());
}

TEST(DecoderBufferTest, FromSharedMemoryHandle_Unaligned) {
  const uint8_t kData[] = "XXXhello";
  const size_t kDataSize = arraysize(kData);
  const off_t kDataOffset = 3;

  base::SharedMemory mem;
  ASSERT_TRUE(mem.CreateAndMapAnonymous(kDataSize));
  memcpy(mem.memory(), kData, kDataSize);

  scoped_refptr<DecoderBuffer> buffer(DecoderBuffer::FromSharedMemoryHandle(
      mem.TakeHandle(), kDataOffset, kDataSize - kDataOffset));
  ASSERT_TRUE(buffer.get());
  EXPECT_EQ(buffer->data_size(), kDataSize - kDataOffset);
  EXPECT_EQ(
      0, memcmp(buffer->data(), kData + kDataOffset, kDataSize - kDataOffset));
  EXPECT_FALSE(buffer->end_of_stream());
  EXPECT_FALSE(buffer->is_key_frame());
}

TEST(DecoderBufferTest, FromSharedMemoryHandle_ZeroSize) {
  const uint8_t kData[] = "hello";
  const size_t kDataSize = arraysize(kData);

  base::SharedMemory mem;
  ASSERT_TRUE(mem.CreateAndMapAnonymous(kDataSize));
  memcpy(mem.memory(), kData, kDataSize);

  scoped_refptr<DecoderBuffer> buffer(
      DecoderBuffer::FromSharedMemoryHandle(mem.TakeHandle(), 0, 0));
  ASSERT_FALSE(buffer.get());
}

#if !defined(OS_ANDROID)
TEST(DecoderBufferTest, PaddingAlignment) {
  const uint8_t kData[] = "hello";
  const size_t kDataSize = arraysize(kData);
  scoped_refptr<DecoderBuffer> buffer2(DecoderBuffer::CopyFrom(
      reinterpret_cast<const uint8_t*>(&kData), kDataSize));
  ASSERT_TRUE(buffer2.get());

  // Padding data should always be zeroed.
  for(int i = 0; i < DecoderBuffer::kPaddingSize; i++)
    EXPECT_EQ((buffer2->data() + kDataSize)[i], 0);

  // If the data is padded correctly we should be able to read and write past
  // the end of the data by DecoderBuffer::kPaddingSize bytes without crashing
  // or Valgrind/ASAN throwing errors.
  const uint8_t kFillChar = 0xFF;
  memset(
      buffer2->writable_data() + kDataSize, kFillChar,
      DecoderBuffer::kPaddingSize);
  for(int i = 0; i < DecoderBuffer::kPaddingSize; i++)
    EXPECT_EQ((buffer2->data() + kDataSize)[i], kFillChar);

  EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(
      buffer2->data()) & (DecoderBuffer::kAlignmentSize - 1));

  EXPECT_FALSE(buffer2->is_key_frame());
}
#endif

TEST(DecoderBufferTest, ReadingWriting) {
  const char kData[] = "hello";
  const size_t kDataSize = arraysize(kData);

  scoped_refptr<DecoderBuffer> buffer(new DecoderBuffer(kDataSize));
  ASSERT_TRUE(buffer.get());

  uint8_t* data = buffer->writable_data();
  ASSERT_TRUE(data);
  ASSERT_EQ(kDataSize, buffer->data_size());
  memcpy(data, kData, kDataSize);
  const uint8_t* read_only_data = buffer->data();
  ASSERT_EQ(data, read_only_data);
  ASSERT_EQ(0, memcmp(read_only_data, kData, kDataSize));
  EXPECT_FALSE(buffer->end_of_stream());
}

TEST(DecoderBufferTest, DecryptConfig) {
  scoped_refptr<DecoderBuffer> buffer(new DecoderBuffer(0));
  EXPECT_FALSE(buffer->decrypt_config());

  const char kKeyId[] = "key id";
  const char kIv[] = "0123456789abcdef";
  std::vector<SubsampleEntry> subsamples;
  subsamples.push_back(SubsampleEntry(10, 5));
  subsamples.push_back(SubsampleEntry(15, 7));

  std::unique_ptr<DecryptConfig> decrypt_config =
      DecryptConfig::CreateCencConfig(kKeyId, kIv, subsamples);

  buffer->set_decrypt_config(
      DecryptConfig::CreateCencConfig(kKeyId, kIv, subsamples));

  EXPECT_TRUE(buffer->decrypt_config());
  EXPECT_TRUE(buffer->decrypt_config()->Matches(*decrypt_config));
}

TEST(DecoderBufferTest, IsKeyFrame) {
  scoped_refptr<DecoderBuffer> buffer(new DecoderBuffer(0));
  EXPECT_FALSE(buffer->is_key_frame());

  buffer->set_is_key_frame(false);
  EXPECT_FALSE(buffer->is_key_frame());

  buffer->set_is_key_frame(true);
  EXPECT_TRUE(buffer->is_key_frame());
}

}  // namespace media
