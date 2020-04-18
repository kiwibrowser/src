/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gtest/gtest.h"

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/debug_stub/transport.h"

static const int kBufSize = 1024;
static const int kDisconnectFlag = -1;

class TransportIPCTests : public ::testing::Test {
 protected:
  port::ITransport *transport;
  int fd[2];
  char buf[kBufSize];

  void SetUp() {
    EXPECT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fd), 0);
    transport = port::CreateTransportIPC(fd[1]);
    memset(buf, 0, kBufSize);
  }

  void TearDown() {
    close(fd[0]);
    delete transport;
  }
};

static const int packet[9] = {
  32,  // Payload size is 32 bytes.
  0, 1, 2, 3, 4, 5, 6, 7
};

static const int packet2[2] = {
  4,  // Payload size is 4 bytes.
  8
};

static const int out_packet[4] = {
  0, 1, 2, 3
};

// Block until you read len bytes.
// Return false on EOF or error, but retries on EINTR.
bool ReadNBytes(int fd, void *ptr, uint32_t len) {
  char *buf = reinterpret_cast<char *>(ptr);
  uint32_t bytes_read = 0;
  while (len > 0) {
    int result = ::read(fd, buf + bytes_read, len);
    if (result > 0) {
      bytes_read += result;
      len -= result;
    } else if (result == 0 || errno != EINTR) {
      return false;
    }
  }
  return true;
}

// Keep trying until you write len bytes.
// Return false on EOF or error, but retries on EINTR.
bool WriteNBytes(int fd, const void *ptr, uint32_t len) {
  const char *buf = reinterpret_cast<const char *>(ptr);
  uint32_t bytes_read = 0;
  while (len > 0) {
    int result = ::write(fd, buf + bytes_read, len);
    if (result > 0) {
      bytes_read += result;
      len -= result;
    } else if (result == 0 || errno != EINTR) {
      return false;
    }
  }
  return true;
}

// Test accepting multiple connections in sequence.
TEST_F(TransportIPCTests, TestAcceptConnection) {
  // Write data so AcceptConnection() wont block.
  EXPECT_EQ(WriteNBytes(fd[0], packet, sizeof(packet)), true);

  // Since there is data this should return true without blocking.
  EXPECT_EQ(transport->AcceptConnection(), true);

  // Write -1 so transport marks itself as disconnected.
  EXPECT_EQ(WriteNBytes(fd[0], &kDisconnectFlag, sizeof(kDisconnectFlag)),
            true);

  // Read data sent and -1 flag.
  EXPECT_EQ(transport->Read(buf, kBufSize), false);

  // Try to establish new connection.
  EXPECT_EQ(WriteNBytes(fd[0], packet, sizeof(packet)), true);
  EXPECT_EQ(transport->AcceptConnection(), true);
}

// Test reading multiple buffered packets at once.
TEST_F(TransportIPCTests, TestReadMultiplePacketsAtOnce) {
  // Write initial data and accept connection.
  EXPECT_EQ(WriteNBytes(fd[0], packet, sizeof(packet)), true);
  EXPECT_EQ(transport->AcceptConnection(), true);

  // Write a second packet.
  EXPECT_EQ(WriteNBytes(fd[0], packet2, sizeof(packet2)), true);

  // Read both packets.
  EXPECT_EQ(transport->Read(buf, 9 * sizeof(int)), true);

  // Check if packets were both read properly.
  for (int i = 0; i < 9; i++) {
    EXPECT_EQ(reinterpret_cast<int*>(buf)[i], i);
  }
}

// Test reading single packet over multiple calls to read.
TEST_F(TransportIPCTests, TestReadSinglePacket) {
  // Write initial data and accept connection.
  EXPECT_EQ(WriteNBytes(fd[0], packet, sizeof(packet)), true);
  EXPECT_EQ(transport->AcceptConnection(), true);

  // Read first half of the packet.
  EXPECT_EQ(transport->Read(buf, 4 * sizeof(int)), true);
  // Read second half of the packet.
  EXPECT_EQ(transport->Read(buf + 16, 4 * sizeof(int)), true);

  // Check if entire packets were both read properly.
  for (int i = 0; i < 8; i++) {
    EXPECT_EQ(reinterpret_cast<int*>(buf)[i], i);
  }
}

// Test IsDataAvailable()
TEST_F(TransportIPCTests, TestIsDataAvailable) {
  // Write initial data and accept connection.
  EXPECT_EQ(WriteNBytes(fd[0], packet, sizeof(packet)), true);
  EXPECT_EQ(transport->AcceptConnection(), true);

  // Should be data available from initial write.
  EXPECT_EQ(transport->IsDataAvailable(), true);

  // Read some of the data
  EXPECT_EQ(transport->Read(buf, 4 * sizeof(int)), true);

  // Should still be data available from initial write.
  EXPECT_EQ(transport->IsDataAvailable(), true);

  // Read the rest of the data
  EXPECT_EQ(transport->Read(buf, 4 * sizeof(int)), true);

  // No more data.
  EXPECT_EQ(transport->IsDataAvailable(), false);
}

// Test writing data.
TEST_F(TransportIPCTests, TestWrite) {
  // Write initial data and accept connection.
  EXPECT_EQ(WriteNBytes(fd[0], packet, sizeof(packet)), true);
  EXPECT_EQ(transport->AcceptConnection(), true);

  // Write packet out.
  EXPECT_EQ(transport->Write(out_packet, sizeof(out_packet)), true);

  EXPECT_EQ(ReadNBytes(fd[0], buf, sizeof(out_packet)), true);

  // Check if out packet was written properly.
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(reinterpret_cast<int*>(buf)[i], i);
  }
}

// Test disconnect.
TEST_F(TransportIPCTests, TestDisconnect) {
  // Write initial data and accept connection.
  EXPECT_TRUE(WriteNBytes(fd[0], packet, sizeof(packet)));
  EXPECT_TRUE(transport->AcceptConnection());

  // Write -1 so transport can disconnect.
  EXPECT_TRUE(WriteNBytes(fd[0], &kDisconnectFlag, sizeof(kDisconnectFlag)));

  // Disconnect and throw away the unread packet.
  transport->Disconnect();

  // Write data so AcceptConnection() wont block.
  EXPECT_TRUE(WriteNBytes(fd[0], packet, sizeof(packet)));
  EXPECT_TRUE(transport->AcceptConnection());

  // Read packet.
  EXPECT_TRUE(transport->Read(buf, sizeof(packet) - 4));

  // Second packet should have been thrown away.
  EXPECT_FALSE(transport->IsDataAvailable());
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
