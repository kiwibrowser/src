// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "chromeos/binder/buffer_reader.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace binder {

TEST(BinderBufferReaderTest, Read) {
  // Prepare data.
  const size_t N = 4;
  int data[N];
  for (size_t i = 0; i < N; ++i) {
    data[i] = i + 100;
  }
  // Read.
  BufferReader reader(reinterpret_cast<char*>(data), sizeof(data));
  for (size_t i = 0; i < N; ++i) {
    SCOPED_TRACE(i);
    EXPECT_TRUE(reader.HasMoreData());
    int value = 0;
    EXPECT_TRUE(reader.Read(&value, sizeof(value)));
    EXPECT_EQ(data[i], value);
  }
  EXPECT_FALSE(reader.HasMoreData());
  // No longer able to read.
  int value = 0;
  EXPECT_FALSE(reader.Read(&value, sizeof(value)));
}

TEST(BinderBufferReaderTest, Skip) {
  // Prepare data.
  const size_t N = 4;
  int data[N];
  for (size_t i = 0; i < N; ++i) {
    data[i] = i + 100;
  }
  // Skip the first and read the rest.
  BufferReader reader(reinterpret_cast<char*>(data), sizeof(data));
  EXPECT_TRUE(reader.HasMoreData());
  EXPECT_TRUE(reader.Skip(sizeof(data[0])));
  for (size_t i = 1; i < N; ++i) {
    SCOPED_TRACE(i);
    EXPECT_TRUE(reader.HasMoreData());
    int value = 0;
    EXPECT_TRUE(reader.Read(&value, sizeof(value)));
    EXPECT_EQ(data[i], value);
  }
  EXPECT_FALSE(reader.HasMoreData());
  // No longer able to skip.
  EXPECT_FALSE(reader.Skip(sizeof(data[0])));
}

}  // namespace binder
