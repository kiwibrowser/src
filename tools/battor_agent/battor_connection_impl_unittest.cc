// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/battor_agent/battor_connection_impl.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "device/serial/test_serial_io_handler.h"
#include "services/device/public/mojom/serial.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/battor_agent/battor_protocol_types.h"

namespace {

void NullWriteCallback(int, device::mojom::SerialSendError) {}
void NullReadCallback(int, device::mojom::SerialReceiveError) {}

}  // namespace

namespace battor {

// TestableBattOrConnection uses a fake serial connection be testable.
class TestableBattOrConnection : public BattOrConnectionImpl {
 public:
  TestableBattOrConnection(BattOrConnection::Listener* listener,
                           const base::TickClock* tick_clock)
      : BattOrConnectionImpl("/dev/test", listener, nullptr) {
    tick_clock_ = tick_clock;
  }
  scoped_refptr<device::SerialIoHandler> CreateIoHandler() override {
    return device::TestSerialIoHandler::Create();
  }

  device::TestSerialIoHandler* GetIoHandler() {
    return reinterpret_cast<device::TestSerialIoHandler*>(io_handler_.get());
  }
};

// BattOrConnectionImplTest provides a BattOrConnection and captures the
// results of all its commands.
class BattOrConnectionImplTest : public testing::Test,
                                 public BattOrConnection::Listener {
 public:
  BattOrConnectionImplTest()
      : task_runner_(new base::TestMockTimeTaskRunner()),
        thread_task_runner_handle_(task_runner_) {}

  void OnConnectionOpened(bool success) override {
    is_open_complete_ = true;
    open_success_ = success;
  };
  void OnConnectionFlushed(bool success) override {
    is_flush_complete_ = true;
    flush_success_ = success;
  };
  void OnBytesSent(bool success) override { send_success_ = success; }
  void OnMessageRead(bool success,
                     BattOrMessageType type,
                     std::unique_ptr<std::vector<char>> bytes) override {
    is_read_complete_ = true;
    read_success_ = success;
    read_type_ = type;
    read_bytes_ = std::move(bytes);
  }

 protected:
  void SetUp() override {
    connection_.reset(
        new TestableBattOrConnection(this, task_runner_->GetMockTickClock()));
    task_runner_->ClearPendingTasks();
  }

  void OpenConnection() {
    is_open_complete_ = false;
    connection_->Open();
    task_runner_->RunUntilIdle();
  }

  void FlushConnection() {
    is_flush_complete_ = false;
    connection_->Flush();
    task_runner_->RunUntilIdle();
  }

  bool IsConnectionOpen() { return connection_->IsOpen(); }

  void CloseConnection() { connection_->Close(); }

  void ReadMessage(BattOrMessageType type) {
    is_read_complete_ = false;
    connection_->ReadMessage(type);
    task_runner_->RunUntilIdle();
  }

  // Reads the specified number of bytes directly from the serial connection.
  scoped_refptr<net::IOBuffer> ReadMessageRaw(int bytes_to_read) {
    scoped_refptr<net::IOBuffer> buffer(
        new net::IOBuffer((size_t)bytes_to_read));

    connection_->GetIoHandler()->Read(std::make_unique<device::ReceiveBuffer>(
        buffer, bytes_to_read, base::BindOnce(&NullReadCallback)));
    task_runner_->RunUntilIdle();

    return buffer;
  }

  void SendControlMessage(BattOrControlMessageType type,
                          uint16_t param1,
                          uint16_t param2) {
    BattOrControlMessage msg{type, param1, param2};
    connection_->SendBytes(BATTOR_MESSAGE_TYPE_CONTROL,
                           reinterpret_cast<char*>(&msg), sizeof(msg));
    task_runner_->RunUntilIdle();
  }

  void ForceReceiveError(device::mojom::SerialReceiveError error) {
    connection_->GetIoHandler()->ForceReceiveError(error);
    task_runner_->RunUntilIdle();
  }

  // Writes the specified bytes directly to the serial connection.
  void SendBytesRaw(const char* data, uint16_t bytes_to_send) {
    connection_->GetIoHandler()->Write(std::make_unique<device::SendBuffer>(
        std::vector<uint8_t>(data, data + bytes_to_send),
        base::BindOnce(&NullWriteCallback)));
    task_runner_->RunUntilIdle();
  }

  void AdvanceTickClock(base::TimeDelta delta) {
    task_runner_->FastForwardBy(delta);
  }

  bool GetOpenSuccess() { return open_success_; }
  bool GetFlushSuccess() { return flush_success_; }
  bool IsOpenComplete() { return is_open_complete_; }
  bool IsFlushComplete() { return is_flush_complete_; }
  bool GetSendSuccess() { return send_success_; }
  bool IsReadComplete() { return is_read_complete_; }
  bool GetReadSuccess() { return read_success_; }
  BattOrMessageType GetReadType() { return read_type_; }
  std::vector<char>* GetReadMessage() { return read_bytes_.get(); }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle thread_task_runner_handle_;

  std::unique_ptr<TestableBattOrConnection> connection_;

  // Result from the last connect command.
  bool open_success_;
  // Results from the last flush command.
  bool flush_success_;
  bool is_open_complete_;
  bool is_flush_complete_;
  // Result from the last send command.
  bool send_success_;
  // Results from the last read command.
  bool is_read_complete_;
  bool read_success_;
  BattOrMessageType read_type_;
  std::unique_ptr<std::vector<char>> read_bytes_;
};

TEST_F(BattOrConnectionImplTest, OpenConnectionSucceedsImmediately) {
  OpenConnection();
  ASSERT_TRUE(IsOpenComplete());
  ASSERT_TRUE(GetOpenSuccess());
  ASSERT_TRUE(IsConnectionOpen());
}

TEST_F(BattOrConnectionImplTest, IsOpenFalseAfterClosingConnection) {
  OpenConnection();
  ASSERT_TRUE(IsOpenComplete());
  ASSERT_TRUE(GetOpenSuccess());
  ASSERT_TRUE(IsConnectionOpen());

  CloseConnection();

  ASSERT_FALSE(IsConnectionOpen());
}

TEST_F(BattOrConnectionImplTest, FlushConnectionSucceedsOnlyAfterTimeout) {
  OpenConnection();
  ASSERT_TRUE(IsOpenComplete());
  ASSERT_TRUE(GetOpenSuccess());

  FlushConnection();
  ASSERT_FALSE(IsFlushComplete());

  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  ASSERT_TRUE(IsFlushComplete());
  ASSERT_TRUE(GetFlushSuccess());
}

TEST_F(BattOrConnectionImplTest, FlushConnectionFlushesAlreadyReadBuffer) {
  OpenConnection();
  ASSERT_TRUE(IsOpenComplete());
  ASSERT_TRUE(GetOpenSuccess());

  // Send two data frames and only read one of them. When reading data frames,
  // we try to read a large chunk from the wire due to the large potential size
  // of a data frame (~100kB). By sending two tiny data frames on the wire and
  // reading back one of them, we know that we read past the end of the first
  // message and all of the second message because the data frames were so
  // small. These extra bytes that were unnecesssary for the first message were
  // storied internally by BattOrConnectionImpl, and we want to ensure that
  // Flush() clears this internal data.
  const char data[] = {
      BATTOR_CONTROL_BYTE_START,
      BATTOR_MESSAGE_TYPE_SAMPLES,
      0x02,
      0x00,
      0x02,
      0x00,
      0x02,
      0x00,
      BATTOR_CONTROL_BYTE_END,
  };
  SendBytesRaw(data, 9);
  SendBytesRaw(data, 9);
  ReadMessage(BATTOR_MESSAGE_TYPE_SAMPLES);

  CloseConnection();
  OpenConnection();
  ASSERT_TRUE(IsOpenComplete());
  ASSERT_TRUE(GetOpenSuccess());

  FlushConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  ReadMessage(BATTOR_MESSAGE_TYPE_SAMPLES);

  // The read should be incomplete due to no data being on the wire - the second
  // control message was cleared by the slow flush.
  ASSERT_FALSE(IsReadComplete());
}

TEST_F(BattOrConnectionImplTest, FlushConnectionNewBytesRestartQuietPeriod) {
  OpenConnection();
  ASSERT_TRUE(IsOpenComplete());
  ASSERT_TRUE(GetOpenSuccess());

  FlushConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(49));
  SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_RESET, 4, 7);
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(49));

  // The connection should not yet be opened because we received new bytes at
  // t=49ms, and at t=98ms the new 50ms quiet period hasn't yet elapsed.
  ASSERT_FALSE(IsFlushComplete());

  AdvanceTickClock(base::TimeDelta::FromMilliseconds(1));

  ASSERT_TRUE(IsFlushComplete());
}

TEST_F(BattOrConnectionImplTest,
       FlushConnectionFlushesBytesReceivedInQuietPeriod) {
  OpenConnection();
  ASSERT_TRUE(IsOpenComplete());
  ASSERT_TRUE(GetOpenSuccess());

  FlushConnection();
  SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_RESET, 4, 7);
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(5));
  ASSERT_FALSE(IsFlushComplete());

  SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_RESET, 4, 7);
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));
  ASSERT_TRUE(IsFlushComplete());
  ASSERT_TRUE(GetFlushSuccess());

  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  // The read should hang because the control message that arrived mid quiet
  // period was thrown out.
  ASSERT_FALSE(IsReadComplete());
}

TEST_F(BattOrConnectionImplTest, FlushConnectionFlushesMultipleReadsOfData) {
  OpenConnection();
  ASSERT_TRUE(IsOpenComplete());
  ASSERT_TRUE(GetOpenSuccess());

  // Send 10 full flush buffers worth of data.
  char data[50000];
  for (size_t i = 0; i < 50000; i++)
    data[i] = '0';
  for (int i = 0; i < 10; i++)
    SendBytesRaw(data, 50000);

  FlushConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_RESET, 4, 7);
  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  // Even though 500kB of garbage data was sent before the valid control
  // message on the serial connection, the slow flush should have cleared it
  // all, resulting in a successful read.
  ASSERT_TRUE(IsReadComplete());
  ASSERT_TRUE(GetReadSuccess());
}

TEST_F(BattOrConnectionImplTest, FlushIncompleteBeforeTimeout) {
  OpenConnection();
  ASSERT_TRUE(IsOpenComplete());
  ASSERT_TRUE(GetOpenSuccess());

  FlushConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(49));

  ASSERT_FALSE(IsFlushComplete());
}

TEST_F(BattOrConnectionImplTest, FlushFailsWithNonTimeoutError) {
  OpenConnection();
  ASSERT_TRUE(IsOpenComplete());
  ASSERT_TRUE(GetOpenSuccess());

  FlushConnection();
  ForceReceiveError(device::mojom::SerialReceiveError::DISCONNECTED);

  ASSERT_FALSE(GetFlushSuccess());
}

TEST_F(BattOrConnectionImplTest, ControlSendEscapesStartBytesCorrectly) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  SendControlMessage(
      BATTOR_CONTROL_MESSAGE_TYPE_INIT,
      BATTOR_CONTROL_BYTE_START,
      BATTOR_CONTROL_BYTE_START);

  const char expected_data[] = {
      BATTOR_CONTROL_BYTE_START, BATTOR_MESSAGE_TYPE_CONTROL,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_MESSAGE_TYPE_INIT,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_BYTE_START,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_BYTE_START,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_END,
  };

  ASSERT_TRUE(GetSendSuccess());
  ASSERT_EQ(0, std::memcmp(ReadMessageRaw(13)->data(), expected_data, 13));
}

TEST_F(BattOrConnectionImplTest, ControlSendEscapesEndBytesCorrectly) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  SendControlMessage(
      BATTOR_CONTROL_MESSAGE_TYPE_RESET,
      BATTOR_CONTROL_BYTE_END,
      BATTOR_CONTROL_BYTE_END);

  const char expected_data[] = {
      BATTOR_CONTROL_BYTE_START, BATTOR_MESSAGE_TYPE_CONTROL,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_MESSAGE_TYPE_RESET,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_BYTE_END,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_BYTE_END,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_END,
  };

  ASSERT_TRUE(GetSendSuccess());
  ASSERT_EQ(0, std::memcmp(ReadMessageRaw(13)->data(), expected_data, 13));
}

TEST_F(BattOrConnectionImplTest, ControlSendEscapesEscapeBytesCorrectly) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  SendControlMessage(
      BATTOR_CONTROL_MESSAGE_TYPE_SELF_TEST,
      BATTOR_CONTROL_BYTE_ESCAPE,
      BATTOR_CONTROL_BYTE_ESCAPE);

  const char expected_data[] = {
      BATTOR_CONTROL_BYTE_START, BATTOR_MESSAGE_TYPE_CONTROL,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_MESSAGE_TYPE_SELF_TEST,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_BYTE_ESCAPE,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_BYTE_ESCAPE,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_END,
  };

  ASSERT_TRUE(GetSendSuccess());
  ASSERT_EQ(0, std::memcmp(ReadMessageRaw(13)->data(), expected_data, 13));
}

TEST_F(BattOrConnectionImplTest, ControlSendEscapesParametersCorrectly) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  // Check if the two control parameters are ordered and escaped properly.
  // This also checks the byte ordering of the two 16-bit control message
  // parameters, which should be little-endian on the wire.
  SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_READ_SD_UART, 0x0100, 0x0002);

  const char expected_data[] = {
      BATTOR_CONTROL_BYTE_START, BATTOR_MESSAGE_TYPE_CONTROL,
      BATTOR_CONTROL_MESSAGE_TYPE_READ_SD_UART,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x01,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x02,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_END,
  };

  ASSERT_TRUE(GetSendSuccess());
  ASSERT_EQ(0, std::memcmp(ReadMessageRaw(12)->data(), expected_data, 12));
}

TEST_F(BattOrConnectionImplTest, InitSendsCorrectBytes) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_INIT, 0, 0);

  const char expected_data[] = {
      BATTOR_CONTROL_BYTE_START,  BATTOR_MESSAGE_TYPE_CONTROL,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_MESSAGE_TYPE_INIT,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_END,
  };

  ASSERT_TRUE(GetSendSuccess());
  ASSERT_EQ(0, std::memcmp(ReadMessageRaw(13)->data(), expected_data, 13));
}

TEST_F(BattOrConnectionImplTest, ResetSendsCorrectBytes) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_RESET, 0, 0);

  const char expected_data[] = {
      BATTOR_CONTROL_BYTE_START,  BATTOR_MESSAGE_TYPE_CONTROL,
      BATTOR_CONTROL_BYTE_ESCAPE, BATTOR_CONTROL_MESSAGE_TYPE_RESET,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_ESCAPE, 0x00,
      BATTOR_CONTROL_BYTE_END,
  };

  ASSERT_TRUE(GetSendSuccess());
  ASSERT_EQ(0, std::memcmp(ReadMessageRaw(13)->data(), expected_data, 13));
}

TEST_F(BattOrConnectionImplTest, ReadMessageControlMessage) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  const char data[] = {
      BATTOR_CONTROL_BYTE_START,
      BATTOR_MESSAGE_TYPE_CONTROL,
      BATTOR_CONTROL_BYTE_ESCAPE,
      BATTOR_CONTROL_MESSAGE_TYPE_RESET,
      0x04,
      0x04,
      0x04,
      0x04,
      BATTOR_CONTROL_BYTE_END,
  };
  SendBytesRaw(data, 9);
  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  const char expected[] = {BATTOR_CONTROL_MESSAGE_TYPE_RESET, 0x04, 0x04, 0x04,
                           0x04};

  ASSERT_TRUE(IsReadComplete());
  ASSERT_TRUE(GetReadSuccess());
  ASSERT_EQ(BATTOR_MESSAGE_TYPE_CONTROL, GetReadType());
  ASSERT_EQ(0, std::memcmp(GetReadMessage()->data(), expected, 5));
}

TEST_F(BattOrConnectionImplTest, ReadMessageInvalidType) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  const char data[] = {
      BATTOR_CONTROL_BYTE_START,
      static_cast<char>(UINT8_MAX),
      BATTOR_CONTROL_BYTE_ESCAPE,
      BATTOR_CONTROL_MESSAGE_TYPE_RESET,
      0x04,
      0x04,
      0x04,
      0x04,
      BATTOR_CONTROL_BYTE_END,
  };
  SendBytesRaw(data, 7);
  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  ASSERT_TRUE(IsReadComplete());
  ASSERT_FALSE(GetReadSuccess());
}

TEST_F(BattOrConnectionImplTest, ReadMessageEndsMidMessage) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  const char data[] = {
      BATTOR_CONTROL_BYTE_START,
      BATTOR_MESSAGE_TYPE_CONTROL,
      BATTOR_CONTROL_BYTE_ESCAPE,
      BATTOR_CONTROL_MESSAGE_TYPE_RESET,
      0x04,
  };
  SendBytesRaw(data, 5);
  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  // The first read should recognize that a second read is necessary.
  ASSERT_FALSE(IsReadComplete());

  ForceReceiveError(device::mojom::SerialReceiveError::TIMEOUT);

  // The second read should fail due to the time out.
  ASSERT_TRUE(IsReadComplete());
  ASSERT_FALSE(GetReadSuccess());
}

TEST_F(BattOrConnectionImplTest, ReadMessageMissingEndByte) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  const char data[] = {
      BATTOR_CONTROL_BYTE_START,
      BATTOR_MESSAGE_TYPE_CONTROL,
      BATTOR_CONTROL_BYTE_ESCAPE,
      BATTOR_CONTROL_MESSAGE_TYPE_RESET,
      0x04,
      0x04,
      0x04,
      0x04,
  };
  SendBytesRaw(data, 6);
  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  // The first read should recognize that a second read is necessary.
  ASSERT_FALSE(IsReadComplete());

  ForceReceiveError(device::mojom::SerialReceiveError::TIMEOUT);

  // The second read should fail due to the time out.
  ASSERT_TRUE(IsReadComplete());
  ASSERT_FALSE(GetReadSuccess());
}

TEST_F(BattOrConnectionImplTest, ReadMessageWithEscapeCharacters) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  const char data[] = {
      BATTOR_CONTROL_BYTE_START,
      BATTOR_MESSAGE_TYPE_CONTROL,
      BATTOR_CONTROL_BYTE_ESCAPE,
      BATTOR_CONTROL_MESSAGE_TYPE_RESET,
      BATTOR_CONTROL_BYTE_ESCAPE,
      0x00,
      0x04,
      0x04,
      0x04,
      BATTOR_CONTROL_BYTE_END,
  };
  SendBytesRaw(data, 10);
  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  const char expected[] = {BATTOR_CONTROL_MESSAGE_TYPE_RESET, 0x00};

  ASSERT_TRUE(IsReadComplete());
  ASSERT_TRUE(GetReadSuccess());
  ASSERT_EQ(BATTOR_MESSAGE_TYPE_CONTROL, GetReadType());
  ASSERT_EQ(0, std::memcmp(GetReadMessage()->data(), expected, 2));
}

TEST_F(BattOrConnectionImplTest, ReadControlMessage) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_RESET, 4, 7);
  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  ASSERT_TRUE(IsReadComplete());
  ASSERT_TRUE(GetReadSuccess());
  ASSERT_EQ(BATTOR_MESSAGE_TYPE_CONTROL, GetReadType());

  BattOrControlMessage* msg =
      reinterpret_cast<BattOrControlMessage*>(GetReadMessage()->data());

  ASSERT_EQ(BATTOR_CONTROL_MESSAGE_TYPE_RESET, msg->type);
  ASSERT_EQ(4, msg->param1);
  ASSERT_EQ(7, msg->param2);
}

TEST_F(BattOrConnectionImplTest, ReadMessageExtraBytesStoredBetweenReads) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  // Send a samples frame with length and sequence number of zero.
  const char data[] = {
      BATTOR_CONTROL_BYTE_START,
      BATTOR_MESSAGE_TYPE_SAMPLES,
      0x02,
      0x00,
      0x02,
      0x00,
      0x02,
      0x00,
      BATTOR_CONTROL_BYTE_END,
  };
  SendBytesRaw(data, 9);
  SendControlMessage(BATTOR_CONTROL_MESSAGE_TYPE_INIT, 5, 8);

  // When reading sample frames, we're forced to read lots because each frame
  // could be up to 50kB long. By reading a really short sample frame (like
  // the zero-length one above), the BattOrConnection is forced to store
  // whatever extra data it finds in the serial stream - in this case, the
  // init control message that we sent.
  ReadMessage(BATTOR_MESSAGE_TYPE_SAMPLES);

  ASSERT_TRUE(IsReadComplete());
  ASSERT_TRUE(GetReadSuccess());
  ASSERT_EQ(BATTOR_MESSAGE_TYPE_SAMPLES, GetReadType());

  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  ASSERT_TRUE(IsReadComplete());
  ASSERT_TRUE(GetReadSuccess());
  ASSERT_EQ(BATTOR_MESSAGE_TYPE_CONTROL, GetReadType());

  BattOrControlMessage* init_msg =
      reinterpret_cast<BattOrControlMessage*>(GetReadMessage()->data());

  ASSERT_EQ(BATTOR_CONTROL_MESSAGE_TYPE_INIT, init_msg->type);
  ASSERT_EQ(5, init_msg->param1);
  ASSERT_EQ(8, init_msg->param2);
}

TEST_F(BattOrConnectionImplTest, ReadMessageFailsWithControlButExpectingAck) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  const char data[] = {
      BATTOR_CONTROL_BYTE_START,
      BATTOR_MESSAGE_TYPE_CONTROL_ACK,
      BATTOR_CONTROL_BYTE_ESCAPE,
      BATTOR_CONTROL_MESSAGE_TYPE_RESET,
      0x04,
      BATTOR_CONTROL_BYTE_END,
  };
  SendBytesRaw(data, 6);
  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  ASSERT_TRUE(IsReadComplete());
  ASSERT_FALSE(GetReadSuccess());
}

TEST_F(BattOrConnectionImplTest, ReadMessageFailsWithAckButExpectingControl) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  const char data[] = {
      BATTOR_CONTROL_BYTE_START,         BATTOR_MESSAGE_TYPE_CONTROL_ACK,
      BATTOR_CONTROL_MESSAGE_TYPE_RESET, 0x04,
      BATTOR_CONTROL_BYTE_END,
  };
  SendBytesRaw(data, 5);
  ReadMessage(BATTOR_MESSAGE_TYPE_CONTROL);

  ASSERT_TRUE(IsReadComplete());
  ASSERT_FALSE(GetReadSuccess());
}

TEST_F(BattOrConnectionImplTest, ReadMessageControlTypePrintFails) {
  OpenConnection();
  AdvanceTickClock(base::TimeDelta::FromMilliseconds(50));

  const char data[] = {
      BATTOR_CONTROL_BYTE_START, BATTOR_MESSAGE_TYPE_PRINT,
      BATTOR_CONTROL_BYTE_END,
  };
  SendBytesRaw(data, 3);
  ReadMessage(BATTOR_MESSAGE_TYPE_PRINT);

  ASSERT_TRUE(IsReadComplete());
  ASSERT_FALSE(GetReadSuccess());
}

}  // namespace battor
