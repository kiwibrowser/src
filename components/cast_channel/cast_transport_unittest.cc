// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_channel/cast_transport.h"

#include <stddef.h>
#include <stdint.h>

#include "base/callback_helpers.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/simple_test_clock.h"
#include "components/cast_channel/cast_framer.h"
#include "components/cast_channel/cast_socket.h"
#include "components/cast_channel/cast_test_util.h"
#include "components/cast_channel/logger.h"
#include "components/cast_channel/proto/cast_channel.pb.h"
#include "net/base/completion_callback.h"
#include "net/base/net_errors.h"
#include "net/log/test_net_log.h"
#include "net/socket/socket.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::DoAll;
using testing::InSequence;
using testing::Invoke;
using testing::NotNull;
using testing::Return;
using testing::WithArg;

namespace cast_channel {
namespace {

const int kChannelId = 0;

// Mockable placeholder for write completion events.
class CompleteHandler {
 public:
  CompleteHandler() {}
  MOCK_METHOD1(Complete, void(int result));

 private:
  DISALLOW_COPY_AND_ASSIGN(CompleteHandler);
};

// Creates a CastMessage proto with the bare minimum required fields set.
CastMessage CreateCastMessage() {
  CastMessage output;
  output.set_protocol_version(CastMessage::CASTV2_1_0);
  output.set_namespace_("x");
  output.set_source_id("source");
  output.set_destination_id("destination");
  output.set_payload_type(CastMessage::STRING);
  output.set_payload_utf8("payload");
  return output;
}

// FIFO queue of completion callbacks. Outstanding write operations are
// Push()ed into the queue. Callback completion is simulated by invoking
// Pop() in the same order as Push().
class CompletionQueue {
 public:
  CompletionQueue() {}
  ~CompletionQueue() { CHECK_EQ(0u, cb_queue_.size()); }

  // Enqueues a pending completion callback.
  void Push(const net::CompletionCallback& cb) { cb_queue_.push(cb); }
  // Runs the next callback and removes it from the queue.
  void Pop(int rv) {
    CHECK_GT(cb_queue_.size(), 0u);
    cb_queue_.front().Run(rv);
    cb_queue_.pop();
  }

 private:
  base::queue<net::CompletionCallback> cb_queue_;
  DISALLOW_COPY_AND_ASSIGN(CompletionQueue);
};

// GMock action that reads data from an IOBuffer and writes it to a string
// variable.
//
//   buf_idx (template parameter 0): 0-based index of the net::IOBuffer
//                                   in the function mock arg list.
//   size_idx (template parameter 1): 0-based index of the byte count arg.
//   str: pointer to the string which will receive data from the buffer.
ACTION_TEMPLATE(ReadBufferToString,
                HAS_2_TEMPLATE_PARAMS(int, buf_idx, int, size_idx),
                AND_1_VALUE_PARAMS(str)) {
  str->assign(testing::get<buf_idx>(args)->data(),
              testing::get<size_idx>(args));
}

// GMock action that writes data from a string to an IOBuffer.
//
//   buf_idx (template parameter 0): 0-based index of the IOBuffer arg.
//   str: the string containing data to be written to the IOBuffer.
ACTION_TEMPLATE(FillBufferFromString,
                HAS_1_TEMPLATE_PARAMS(int, buf_idx),
                AND_1_VALUE_PARAMS(str)) {
  memcpy(testing::get<buf_idx>(args)->data(), str.data(), str.size());
}

// GMock action that enqueues a write completion callback in a queue.
//
//   buf_idx (template parameter 0): 0-based index of the CompletionCallback.
//   completion_queue: a pointer to the CompletionQueue.
ACTION_TEMPLATE(EnqueueCallback,
                HAS_1_TEMPLATE_PARAMS(int, cb_idx),
                AND_1_VALUE_PARAMS(completion_queue)) {
  completion_queue->Push(testing::get<cb_idx>(args));
}

}  // namespace

class MockSocket : public net::Socket {
 public:
  int Read(net::IOBuffer* buffer,
           int bytes,
           net::CompletionOnceCallback callback) override {
    return Read(buffer, bytes,
                base::AdaptCallbackForRepeating(std::move(callback)));
  }

  int Write(net::IOBuffer* buffer,
            int bytes,
            net::CompletionOnceCallback callback,
            const net::NetworkTrafficAnnotationTag& tag) override {
    return Write(buffer, bytes,
                 base::AdaptCallbackForRepeating(std::move(callback)), tag);
  }

  MOCK_METHOD3(Read,
               int(net::IOBuffer* buf,
                   int buf_len,
                   const net::CompletionCallback& callback));

  MOCK_METHOD4(Write,
               int(net::IOBuffer* buf,
                   int buf_len,
                   const net::CompletionCallback& callback,
                   const net::NetworkTrafficAnnotationTag&));

  int SetReceiveBufferSize(int32_t size) override {
    NOTREACHED();
    return 0;
  }

  int SetSendBufferSize(int32_t size) override {
    NOTREACHED();
    return 0;
  }
};

class CastTransportTest : public testing::Test {
 public:
  CastTransportTest() : logger_(new Logger()) {
    delegate_ = new MockCastTransportDelegate;
    transport_.reset(new CastTransportImpl(&mock_socket_, kChannelId,
                                           CreateIPEndPointForTest(), logger_));
    transport_->SetReadDelegate(base::WrapUnique(delegate_));
  }
  ~CastTransportTest() override {}

 protected:
  // Runs all pending tasks in the message loop.
  void RunPendingTasks() {
    base::RunLoop run_loop;
    run_loop.RunUntilIdle();
  }

  base::MessageLoop message_loop_;
  MockCastTransportDelegate* delegate_;
  MockSocket mock_socket_;
  Logger* logger_;
  std::unique_ptr<CastTransport> transport_;
};

// ----------------------------------------------------------------------------
// Asynchronous write tests
TEST_F(CastTransportTest, TestFullWriteAsync) {
  CompletionQueue socket_cbs;
  CompleteHandler write_handler;
  std::string output;

  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  EXPECT_CALL(mock_socket_, Write(NotNull(), serialized_message.size(), _, _))
      .WillOnce(DoAll(ReadBufferToString<0, 1>(&output),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)));
  EXPECT_CALL(write_handler, Complete(net::OK));
  transport_->SendMessage(
      message,
      base::Bind(&CompleteHandler::Complete, base::Unretained(&write_handler)),
      TRAFFIC_ANNOTATION_FOR_TESTS);
  RunPendingTasks();
  socket_cbs.Pop(serialized_message.size());
  RunPendingTasks();
  EXPECT_EQ(serialized_message, output);
}

TEST_F(CastTransportTest, TestPartialWritesAsync) {
  InSequence seq;
  CompletionQueue socket_cbs;
  CompleteHandler write_handler;
  std::string output;

  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  // Only one byte is written.
  EXPECT_CALL(
      mock_socket_,
      Write(NotNull(), static_cast<int>(serialized_message.size()), _, _))
      .WillOnce(DoAll(ReadBufferToString<0, 1>(&output),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)));
  // Remainder of bytes are written.
  EXPECT_CALL(
      mock_socket_,
      Write(NotNull(), static_cast<int>(serialized_message.size() - 1), _, _))
      .WillOnce(DoAll(ReadBufferToString<0, 1>(&output),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)));

  transport_->SendMessage(
      message,
      base::Bind(&CompleteHandler::Complete, base::Unretained(&write_handler)),
      TRAFFIC_ANNOTATION_FOR_TESTS);
  RunPendingTasks();
  EXPECT_EQ(serialized_message, output);
  socket_cbs.Pop(1);
  RunPendingTasks();

  EXPECT_CALL(write_handler, Complete(net::OK));
  socket_cbs.Pop(serialized_message.size() - 1);
  RunPendingTasks();
  EXPECT_EQ(serialized_message.substr(1, serialized_message.size() - 1),
            output);
}

TEST_F(CastTransportTest, TestWriteFailureAsync) {
  CompletionQueue socket_cbs;
  CompleteHandler write_handler;
  CastMessage message = CreateCastMessage();
  EXPECT_CALL(mock_socket_, Write(NotNull(), _, _, _))
      .WillOnce(
          DoAll(EnqueueCallback<2>(&socket_cbs), Return(net::ERR_IO_PENDING)));
  EXPECT_CALL(write_handler, Complete(net::ERR_FAILED));
  EXPECT_CALL(*delegate_, OnError(ChannelError::CAST_SOCKET_ERROR));
  transport_->SendMessage(
      message,
      base::Bind(&CompleteHandler::Complete, base::Unretained(&write_handler)),
      TRAFFIC_ANNOTATION_FOR_TESTS);
  RunPendingTasks();
  socket_cbs.Pop(net::ERR_CONNECTION_RESET);
  RunPendingTasks();
  EXPECT_EQ(ChannelEvent::SOCKET_WRITE,
            logger_->GetLastError(kChannelId).channel_event);
  EXPECT_EQ(net::ERR_CONNECTION_RESET,
            logger_->GetLastError(kChannelId).net_return_value);
}

// ----------------------------------------------------------------------------
// Synchronous write tests
TEST_F(CastTransportTest, TestFullWriteSync) {
  CompleteHandler write_handler;
  std::string output;
  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));
  EXPECT_CALL(mock_socket_, Write(NotNull(), serialized_message.size(), _, _))
      .WillOnce(DoAll(ReadBufferToString<0, 1>(&output),
                      Return(serialized_message.size())));
  EXPECT_CALL(write_handler, Complete(net::OK));
  transport_->SendMessage(
      message,
      base::Bind(&CompleteHandler::Complete, base::Unretained(&write_handler)),
      TRAFFIC_ANNOTATION_FOR_TESTS);
  RunPendingTasks();
  EXPECT_EQ(serialized_message, output);
}

TEST_F(CastTransportTest, TestPartialWritesSync) {
  InSequence seq;
  CompleteHandler write_handler;
  std::string output;

  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  // Only one byte is written.
  EXPECT_CALL(mock_socket_, Write(NotNull(), serialized_message.size(), _, _))
      .WillOnce(DoAll(ReadBufferToString<0, 1>(&output), Return(1)));
  // Remainder of bytes are written.
  EXPECT_CALL(mock_socket_,
              Write(NotNull(), serialized_message.size() - 1, _, _))
      .WillOnce(DoAll(ReadBufferToString<0, 1>(&output),
                      Return(serialized_message.size() - 1)));

  EXPECT_CALL(write_handler, Complete(net::OK));
  transport_->SendMessage(
      message,
      base::Bind(&CompleteHandler::Complete, base::Unretained(&write_handler)),
      TRAFFIC_ANNOTATION_FOR_TESTS);
  RunPendingTasks();
  EXPECT_EQ(serialized_message.substr(1, serialized_message.size() - 1),
            output);
}

TEST_F(CastTransportTest, TestWriteFailureSync) {
  CompleteHandler write_handler;
  CastMessage message = CreateCastMessage();
  EXPECT_CALL(mock_socket_, Write(NotNull(), _, _, _))
      .WillOnce(Return(net::ERR_CONNECTION_RESET));
  EXPECT_CALL(write_handler, Complete(net::ERR_FAILED));
  transport_->SendMessage(
      message,
      base::Bind(&CompleteHandler::Complete, base::Unretained(&write_handler)),
      TRAFFIC_ANNOTATION_FOR_TESTS);
  RunPendingTasks();
  EXPECT_EQ(ChannelEvent::SOCKET_WRITE,
            logger_->GetLastError(kChannelId).channel_event);
  EXPECT_EQ(net::ERR_CONNECTION_RESET,
            logger_->GetLastError(kChannelId).net_return_value);
}

// ----------------------------------------------------------------------------
// Asynchronous read tests
TEST_F(CastTransportTest, TestFullReadAsync) {
  InSequence s;
  CompletionQueue socket_cbs;

  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));
  EXPECT_CALL(*delegate_, Start());

  // Read bytes [0, 3].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)));

  // Read bytes [4, n].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(),
                   serialized_message.size() -
                       MessageFramer::MessageHeader::header_size(),
                   _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message.substr(
                          MessageFramer::MessageHeader::header_size(),
                          serialized_message.size() -
                              MessageFramer::MessageHeader::header_size())),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)))
      .RetiresOnSaturation();

  EXPECT_CALL(*delegate_, OnMessage(EqualsProto(message)));
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(Return(net::ERR_IO_PENDING));
  transport_->Start();
  RunPendingTasks();
  socket_cbs.Pop(MessageFramer::MessageHeader::header_size());
  socket_cbs.Pop(serialized_message.size() -
                 MessageFramer::MessageHeader::header_size());
  RunPendingTasks();
}

TEST_F(CastTransportTest, TestPartialReadAsync) {
  InSequence s;
  CompletionQueue socket_cbs;

  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  EXPECT_CALL(*delegate_, Start());

  // Read bytes [0, 3].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)))
      .RetiresOnSaturation();
  // Read bytes [4, n-1].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(),
                   serialized_message.size() -
                       MessageFramer::MessageHeader::header_size(),
                   _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message.substr(
                          MessageFramer::MessageHeader::header_size(),
                          serialized_message.size() -
                              MessageFramer::MessageHeader::header_size() - 1)),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)))
      .RetiresOnSaturation();
  // Read final byte.
  EXPECT_CALL(mock_socket_, Read(NotNull(), 1, _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message.substr(
                          serialized_message.size() - 1, 1)),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)))
      .RetiresOnSaturation();
  EXPECT_CALL(*delegate_, OnMessage(EqualsProto(message)));
  transport_->Start();
  socket_cbs.Pop(MessageFramer::MessageHeader::header_size());
  socket_cbs.Pop(serialized_message.size() -
                 MessageFramer::MessageHeader::header_size() - 1);
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(Return(net::ERR_IO_PENDING));
  socket_cbs.Pop(1);
}

TEST_F(CastTransportTest, TestReadErrorInHeaderAsync) {
  CompletionQueue socket_cbs;

  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  EXPECT_CALL(*delegate_, Start());

  // Read bytes [0, 3].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)))
      .RetiresOnSaturation();

  EXPECT_CALL(*delegate_, OnError(ChannelError::CAST_SOCKET_ERROR));
  transport_->Start();
  // Header read failure.
  socket_cbs.Pop(net::ERR_CONNECTION_RESET);
  EXPECT_EQ(ChannelEvent::SOCKET_READ,
            logger_->GetLastError(kChannelId).channel_event);
  EXPECT_EQ(net::ERR_CONNECTION_RESET,
            logger_->GetLastError(kChannelId).net_return_value);
}

TEST_F(CastTransportTest, TestReadErrorInBodyAsync) {
  CompletionQueue socket_cbs;

  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  EXPECT_CALL(*delegate_, Start());

  // Read bytes [0, 3].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)))
      .RetiresOnSaturation();
  // Read bytes [4, n-1].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(),
                   serialized_message.size() -
                       MessageFramer::MessageHeader::header_size(),
                   _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message.substr(
                          MessageFramer::MessageHeader::header_size(),
                          serialized_message.size() -
                              MessageFramer::MessageHeader::header_size() - 1)),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)))
      .RetiresOnSaturation();
  EXPECT_CALL(*delegate_, OnError(ChannelError::CAST_SOCKET_ERROR));
  transport_->Start();
  // Header read is OK.
  socket_cbs.Pop(MessageFramer::MessageHeader::header_size());
  // Body read fails.
  socket_cbs.Pop(net::ERR_CONNECTION_RESET);
  EXPECT_EQ(ChannelEvent::SOCKET_READ,
            logger_->GetLastError(kChannelId).channel_event);
  EXPECT_EQ(net::ERR_CONNECTION_RESET,
            logger_->GetLastError(kChannelId).net_return_value);
}

TEST_F(CastTransportTest, TestReadCorruptedMessageAsync) {
  CompletionQueue socket_cbs;

  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  // Corrupt the serialized message body(set it to X's).
  for (size_t i = MessageFramer::MessageHeader::header_size();
       i < serialized_message.size(); ++i) {
    serialized_message[i] = 'x';
  }

  EXPECT_CALL(*delegate_, Start());
  // Read bytes [0, 3].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)))
      .RetiresOnSaturation();
  // Read bytes [4, n].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(),
                   serialized_message.size() -
                       MessageFramer::MessageHeader::header_size(),
                   _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message.substr(
                          MessageFramer::MessageHeader::header_size(),
                          serialized_message.size() -
                              MessageFramer::MessageHeader::header_size() - 1)),
                      EnqueueCallback<2>(&socket_cbs),
                      Return(net::ERR_IO_PENDING)))
      .RetiresOnSaturation();

  EXPECT_CALL(*delegate_, OnError(ChannelError::INVALID_MESSAGE));
  transport_->Start();
  socket_cbs.Pop(MessageFramer::MessageHeader::header_size());
  socket_cbs.Pop(serialized_message.size() -
                 MessageFramer::MessageHeader::header_size());
}

// ----------------------------------------------------------------------------
// Synchronous read tests
TEST_F(CastTransportTest, TestFullReadSync) {
  InSequence s;
  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  EXPECT_CALL(*delegate_, Start());

  // Read bytes [0, 3].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message),
                      Return(MessageFramer::MessageHeader::header_size())))
      .RetiresOnSaturation();
  // Read bytes [4, n].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(),
                   serialized_message.size() -
                       MessageFramer::MessageHeader::header_size(),
                   _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message.substr(
                          MessageFramer::MessageHeader::header_size(),
                          serialized_message.size() -
                              MessageFramer::MessageHeader::header_size())),
                      Return(serialized_message.size() -
                             MessageFramer::MessageHeader::header_size())))
      .RetiresOnSaturation();
  EXPECT_CALL(*delegate_, OnMessage(EqualsProto(message)));
  // Async result in order to discontinue the read loop.
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(Return(net::ERR_IO_PENDING));
  transport_->Start();
}

TEST_F(CastTransportTest, TestPartialReadSync) {
  InSequence s;

  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  EXPECT_CALL(*delegate_, Start());

  // Read bytes [0, 3].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message),
                      Return(MessageFramer::MessageHeader::header_size())))
      .RetiresOnSaturation();
  // Read bytes [4, n-1].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(),
                   serialized_message.size() -
                       MessageFramer::MessageHeader::header_size(),
                   _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message.substr(
                          MessageFramer::MessageHeader::header_size(),
                          serialized_message.size() -
                              MessageFramer::MessageHeader::header_size() - 1)),
                      Return(serialized_message.size() -
                             MessageFramer::MessageHeader::header_size() - 1)))
      .RetiresOnSaturation();
  // Read final byte.
  EXPECT_CALL(mock_socket_, Read(NotNull(), 1, _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message.substr(
                          serialized_message.size() - 1, 1)),
                      Return(1)))
      .RetiresOnSaturation();
  EXPECT_CALL(*delegate_, OnMessage(EqualsProto(message)));
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(Return(net::ERR_IO_PENDING));
  transport_->Start();
}

TEST_F(CastTransportTest, TestReadErrorInHeaderSync) {
  InSequence s;
  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  EXPECT_CALL(*delegate_, Start());

  // Read bytes [0, 3].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message),
                      Return(net::ERR_CONNECTION_RESET)))
      .RetiresOnSaturation();
  EXPECT_CALL(*delegate_, OnError(ChannelError::CAST_SOCKET_ERROR));
  transport_->Start();
}

TEST_F(CastTransportTest, TestReadErrorInBodySync) {
  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  EXPECT_CALL(*delegate_, Start());

  // Read bytes [0, 3].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message),
                      Return(MessageFramer::MessageHeader::header_size())))
      .RetiresOnSaturation();
  // Read bytes [4, n-1].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(),
                   serialized_message.size() -
                       MessageFramer::MessageHeader::header_size(),
                   _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message.substr(
                          MessageFramer::MessageHeader::header_size(),
                          serialized_message.size() -
                              MessageFramer::MessageHeader::header_size() - 1)),
                      Return(net::ERR_CONNECTION_RESET)))
      .RetiresOnSaturation();
  EXPECT_CALL(*delegate_, OnError(ChannelError::CAST_SOCKET_ERROR));
  transport_->Start();
  EXPECT_EQ(ChannelEvent::SOCKET_READ,
            logger_->GetLastError(kChannelId).channel_event);
  EXPECT_EQ(net::ERR_CONNECTION_RESET,
            logger_->GetLastError(kChannelId).net_return_value);
}

TEST_F(CastTransportTest, TestReadCorruptedMessageSync) {
  InSequence s;
  CastMessage message = CreateCastMessage();
  std::string serialized_message;
  EXPECT_TRUE(MessageFramer::Serialize(message, &serialized_message));

  // Corrupt the serialized message body(set it to X's).
  for (size_t i = MessageFramer::MessageHeader::header_size();
       i < serialized_message.size(); ++i) {
    serialized_message[i] = 'x';
  }

  EXPECT_CALL(*delegate_, Start());

  // Read bytes [0, 3].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(), MessageFramer::MessageHeader::header_size(), _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message),
                      Return(MessageFramer::MessageHeader::header_size())))
      .RetiresOnSaturation();
  // Read bytes [4, n].
  EXPECT_CALL(mock_socket_,
              Read(NotNull(),
                   serialized_message.size() -
                       MessageFramer::MessageHeader::header_size(),
                   _))
      .WillOnce(DoAll(FillBufferFromString<0>(serialized_message.substr(
                          MessageFramer::MessageHeader::header_size(),
                          serialized_message.size() -
                              MessageFramer::MessageHeader::header_size() - 1)),
                      Return(serialized_message.size() -
                             MessageFramer::MessageHeader::header_size())))
      .RetiresOnSaturation();
  EXPECT_CALL(*delegate_, OnError(ChannelError::INVALID_MESSAGE));
  transport_->Start();
}
}  // namespace cast_channel
