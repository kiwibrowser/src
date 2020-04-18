// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/pairing/message_buffer.h"

#include "net/base/io_buffer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pairing_chromeos {

typedef testing::Test MessageBufferTest;

TEST_F(MessageBufferTest, BasicReadWrite) {
  MessageBuffer message_buffer;
  scoped_refptr<net::IOBuffer> io_buffer(new net::IOBuffer(3));
  io_buffer->data()[0] = 3;
  io_buffer->data()[1] = 1;
  io_buffer->data()[2] = 4;

  message_buffer.AddIOBuffer(io_buffer, 3);

  EXPECT_EQ(message_buffer.AvailableBytes(), 3);
  char data = 0;
  message_buffer.ReadBytes(&data, 1);
  EXPECT_EQ(data, 3);
  EXPECT_EQ(message_buffer.AvailableBytes(), 2);
  message_buffer.ReadBytes(&data, 1);
  EXPECT_EQ(data, 1);
  EXPECT_EQ(message_buffer.AvailableBytes(), 1);
  message_buffer.ReadBytes(&data, 1);
  EXPECT_EQ(data, 4);
  EXPECT_EQ(message_buffer.AvailableBytes(), 0);
}

TEST_F(MessageBufferTest, SplitBuffer) {
  MessageBuffer message_buffer;
  scoped_refptr<net::IOBuffer> io_buffer0(new net::IOBuffer(1));
  io_buffer0->data()[0] = 3;

  scoped_refptr<net::IOBuffer> io_buffer1(new net::IOBuffer(2));
  io_buffer1->data()[0] = 1;
  io_buffer1->data()[1] = 4;

  message_buffer.AddIOBuffer(io_buffer0, 1);
  message_buffer.AddIOBuffer(io_buffer1, 2);

  EXPECT_EQ(message_buffer.AvailableBytes(), 3);
  char data[3];
  message_buffer.ReadBytes(data, 3);
  EXPECT_EQ(message_buffer.AvailableBytes(), 0);
  EXPECT_EQ(data[0], 3);
  EXPECT_EQ(data[1], 1);
  EXPECT_EQ(data[2], 4);
}

TEST_F(MessageBufferTest, EmptyBuffer) {
  MessageBuffer message_buffer;
  scoped_refptr<net::IOBuffer> io_buffer0(new net::IOBuffer(1));
  io_buffer0->data()[0] = 3;

  scoped_refptr<net::IOBuffer> io_buffer1(new net::IOBuffer(0));
  scoped_refptr<net::IOBuffer> io_buffer2(new net::IOBuffer(2));
  io_buffer2->data()[0] = 1;
  io_buffer2->data()[1] = 4;

  message_buffer.AddIOBuffer(io_buffer0, 1);
  message_buffer.AddIOBuffer(io_buffer1, 0);
  message_buffer.AddIOBuffer(io_buffer2, 2);

  EXPECT_EQ(message_buffer.AvailableBytes(), 3);
  char data = 0;
  message_buffer.ReadBytes(&data, 1);
  EXPECT_EQ(data, 3);
  EXPECT_EQ(message_buffer.AvailableBytes(), 2);
  message_buffer.ReadBytes(&data, 1);
  EXPECT_EQ(data, 1);
  EXPECT_EQ(message_buffer.AvailableBytes(), 1);
  message_buffer.ReadBytes(&data, 1);
  EXPECT_EQ(data, 4);
  EXPECT_EQ(message_buffer.AvailableBytes(), 0);
}

}  // namespace pairing_chromeos
