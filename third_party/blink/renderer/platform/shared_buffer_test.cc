/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/shared_buffer.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include "base/memory/scoped_refptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

TEST(SharedBufferTest, getAsBytes) {
  char test_data0[] = "Hello";
  char test_data1[] = "World";
  char test_data2[] = "Goodbye";

  scoped_refptr<SharedBuffer> shared_buffer =
      SharedBuffer::Create(test_data0, strlen(test_data0));
  shared_buffer->Append(test_data1, strlen(test_data1));
  shared_buffer->Append(test_data2, strlen(test_data2));

  const size_t size = shared_buffer->size();
  auto data = std::make_unique<char[]>(size);
  ASSERT_TRUE(shared_buffer->GetBytes(data.get(), size));

  char expected_concatenation[] = "HelloWorldGoodbye";
  ASSERT_EQ(strlen(expected_concatenation), size);
  EXPECT_EQ(0, memcmp(expected_concatenation, data.get(),
                      strlen(expected_concatenation)));
}

TEST(SharedBufferTest, getPartAsBytes) {
  char test_data0[] = "Hello";
  char test_data1[] = "World";
  char test_data2[] = "Goodbye";

  scoped_refptr<SharedBuffer> shared_buffer =
      SharedBuffer::Create(test_data0, strlen(test_data0));
  shared_buffer->Append(test_data1, strlen(test_data1));
  shared_buffer->Append(test_data2, strlen(test_data2));

  struct TestData {
    size_t size;
    const char* expected;
  } test_data[] = {
      {17, "HelloWorldGoodbye"}, {7, "HelloWo"}, {3, "Hel"},
  };
  for (TestData& test : test_data) {
    auto data = std::make_unique<char[]>(test.size);
    ASSERT_TRUE(shared_buffer->GetBytes(data.get(), test.size));
    EXPECT_EQ(0, memcmp(test.expected, data.get(), test.size));
  }
}

TEST(SharedBufferTest, getAsBytesLargeSegments) {
  Vector<char> vector0(0x4000);
  for (size_t i = 0; i < vector0.size(); ++i)
    vector0[i] = 'a';
  Vector<char> vector1(0x4000);
  for (size_t i = 0; i < vector1.size(); ++i)
    vector1[i] = 'b';
  Vector<char> vector2(0x4000);
  for (size_t i = 0; i < vector2.size(); ++i)
    vector2[i] = 'c';

  scoped_refptr<SharedBuffer> shared_buffer =
      SharedBuffer::AdoptVector(vector0);
  shared_buffer->Append(vector1);
  shared_buffer->Append(vector2);

  const size_t size = shared_buffer->size();
  auto data = std::make_unique<char[]>(size);
  ASSERT_TRUE(shared_buffer->GetBytes(data.get(), size));

  ASSERT_EQ(0x4000U + 0x4000U + 0x4000U, size);
  int position = 0;
  for (int i = 0; i < 0x4000; ++i) {
    EXPECT_EQ('a', data[position]);
    ++position;
  }
  for (int i = 0; i < 0x4000; ++i) {
    EXPECT_EQ('b', data[position]);
    ++position;
  }
  for (int i = 0; i < 0x4000; ++i) {
    EXPECT_EQ('c', data[position]);
    ++position;
  }
}

TEST(SharedBufferTest, copy) {
  Vector<char> test_data(10000);
  std::generate(test_data.begin(), test_data.end(), &std::rand);

  size_t length = test_data.size();
  scoped_refptr<SharedBuffer> shared_buffer =
      SharedBuffer::Create(test_data.data(), length);
  shared_buffer->Append(test_data.data(), length);
  shared_buffer->Append(test_data.data(), length);
  shared_buffer->Append(test_data.data(), length);
  // sharedBuffer must contain data more than segmentSize (= 0x1000) to check
  // copy().
  ASSERT_EQ(length * 4, shared_buffer->size());

  Vector<char> clone = shared_buffer->Copy();
  ASSERT_EQ(length * 4, clone.size());
  const Vector<char> contiguous = shared_buffer->Copy();
  ASSERT_EQ(contiguous.size(), shared_buffer->size());
  ASSERT_EQ(0, memcmp(clone.data(), contiguous.data(), clone.size()));

  clone.Append(test_data.data(), length);
  ASSERT_EQ(length * 5, clone.size());
}

TEST(SharedBufferTest, constructorWithSizeOnly) {
  size_t length = 10000;
  scoped_refptr<SharedBuffer> shared_buffer = SharedBuffer::Create(length);
  ASSERT_EQ(length, shared_buffer->size());

  // The internal flat buffer should have been resized to |length| therefore
  // getSomeData() should directly return the full size.
  const char* data;
  ASSERT_EQ(length, shared_buffer->GetSomeData(data, static_cast<size_t>(0u)));
}

TEST(SharedBufferTest, FlatData) {
  auto check_flat_data = [](scoped_refptr<const SharedBuffer> shared_buffer) {
    const SharedBuffer::DeprecatedFlatData flat_buffer(shared_buffer);

    EXPECT_EQ(shared_buffer->size(), flat_buffer.size());
    shared_buffer->ForEachSegment([&flat_buffer](
                                      const char* segment, size_t segment_size,
                                      size_t segment_offset) -> bool {
      EXPECT_EQ(
          memcmp(segment, flat_buffer.Data() + segment_offset, segment_size),
          0);

      // If the SharedBuffer is not segmented, FlatData doesn't copy any data.
      EXPECT_EQ(segment_size == flat_buffer.size(),
                segment == flat_buffer.Data());
      return true;
    });
  };

  scoped_refptr<SharedBuffer> shared_buffer = SharedBuffer::Create();

  // Add enough data to hit a couple of segments.
  while (shared_buffer->size() < 10000) {
    check_flat_data(shared_buffer);
    shared_buffer->Append("FooBarBaz", 9u);
  }
}

}  // namespace blink
