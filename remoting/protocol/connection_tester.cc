// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/connection_tester.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "remoting/proto/video.pb.h"
#include "remoting/protocol/message_pipe.h"
#include "remoting/protocol/message_serialization.h"
#include "remoting/protocol/p2p_datagram_socket.h"
#include "remoting/protocol/p2p_stream_socket.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {
namespace protocol {

StreamConnectionTester::StreamConnectionTester(P2PStreamSocket* client_socket,
                                               P2PStreamSocket* host_socket,
                                               int message_size,
                                               int message_count)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      host_socket_(host_socket),
      client_socket_(client_socket),
      message_size_(message_size),
      test_data_size_(message_size * message_count),
      done_(false),
      write_errors_(0),
      read_errors_(0) {}

StreamConnectionTester::~StreamConnectionTester() = default;

void StreamConnectionTester::Start() {
  InitBuffers();
  DoRead();
  DoWrite();
}

void StreamConnectionTester::CheckResults() {
  EXPECT_EQ(0, write_errors_);
  EXPECT_EQ(0, read_errors_);

  ASSERT_EQ(test_data_size_, input_buffer_->offset());

  output_buffer_->SetOffset(0);
  ASSERT_EQ(test_data_size_, output_buffer_->size());

  EXPECT_EQ(0, memcmp(output_buffer_->data(),
                      input_buffer_->StartOfBuffer(), test_data_size_));
}

void StreamConnectionTester::Done() {
  done_ = true;
  task_runner_->PostTask(FROM_HERE,
                         base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
}

void StreamConnectionTester::InitBuffers() {
  output_buffer_ = new net::DrainableIOBuffer(
      new net::IOBuffer(test_data_size_), test_data_size_);
  for (int i = 0; i < test_data_size_; ++i) {
    output_buffer_->data()[i] = static_cast<char>(i);
  }

  input_buffer_ = new net::GrowableIOBuffer();
}

void StreamConnectionTester::DoWrite() {
  int result = 1;
  while (result > 0) {
    if (output_buffer_->BytesRemaining() == 0)
      break;

    int bytes_to_write = std::min(output_buffer_->BytesRemaining(),
                                  message_size_);
    result = client_socket_->Write(
        output_buffer_.get(), bytes_to_write,
        base::Bind(&StreamConnectionTester::OnWritten, base::Unretained(this)),
        TRAFFIC_ANNOTATION_FOR_TESTS);
    HandleWriteResult(result);
  }
}

void StreamConnectionTester::OnWritten(int result) {
  HandleWriteResult(result);
  DoWrite();
}

void StreamConnectionTester::HandleWriteResult(int result) {
  if (result <= 0 && result != net::ERR_IO_PENDING) {
    LOG(ERROR) << "Received error " << result << " when trying to write";
    write_errors_++;
    Done();
  } else if (result > 0) {
    output_buffer_->DidConsume(result);
  }
}

void StreamConnectionTester::DoRead() {
  int result = 1;
  while (result > 0) {
    input_buffer_->SetCapacity(input_buffer_->offset() + message_size_);
    result = host_socket_->Read(
        input_buffer_.get(),
        message_size_,
        base::Bind(&StreamConnectionTester::OnRead, base::Unretained(this)));
    HandleReadResult(result);
  };
}

void StreamConnectionTester::OnRead(int result) {
  HandleReadResult(result);
  if (!done_)
    DoRead();  // Don't try to read again when we are done reading.
}

void StreamConnectionTester::HandleReadResult(int result) {
  if (result <= 0 && result != net::ERR_IO_PENDING) {
    LOG(ERROR) << "Received error " << result << " when trying to read";
    read_errors_++;
    Done();
  } else if (result > 0) {
    // Allocate memory for the next read.
    input_buffer_->set_offset(input_buffer_->offset() + result);
    if (input_buffer_->offset() == test_data_size_)
      Done();
  }
}

DatagramConnectionTester::DatagramConnectionTester(
    P2PDatagramSocket* client_socket,
    P2PDatagramSocket* host_socket,
    int message_size,
    int message_count,
    int delay_ms)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      host_socket_(host_socket),
      client_socket_(client_socket),
      message_size_(message_size),
      message_count_(message_count),
      delay_ms_(delay_ms),
      done_(false),
      write_errors_(0),
      read_errors_(0),
      packets_sent_(0),
      packets_received_(0),
      bad_packets_received_(0) {
  sent_packets_.resize(message_count_);
}

DatagramConnectionTester::~DatagramConnectionTester() = default;

void DatagramConnectionTester::Start() {
  DoRead();
  DoWrite();
}

void DatagramConnectionTester::CheckResults() {
  EXPECT_EQ(0, write_errors_);
  EXPECT_EQ(0, read_errors_);

  EXPECT_EQ(0, bad_packets_received_);

  // Verify that we've received at least one packet.
  EXPECT_GT(packets_received_, 0);
  VLOG(0) << "Received " << packets_received_ << " packets out of "
          << message_count_;
}

void DatagramConnectionTester::Done() {
  done_ = true;
  task_runner_->PostTask(FROM_HERE,
                         base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
}

void DatagramConnectionTester::DoWrite() {
  if (packets_sent_ >= message_count_) {
    Done();
    return;
  }

  scoped_refptr<net::IOBuffer> packet(new net::IOBuffer(message_size_));
  for (int i = 0; i < message_size_; ++i) {
    packet->data()[i] = static_cast<char>(i);
  }
  sent_packets_[packets_sent_] = packet;
  // Put index of this packet in the beginning of the packet body.
  memcpy(packet->data(), &packets_sent_, sizeof(packets_sent_));

  int result = client_socket_->Send(
      packet.get(), message_size_,
      base::Bind(&DatagramConnectionTester::OnWritten, base::Unretained(this)));
  HandleWriteResult(result);
}

void DatagramConnectionTester::OnWritten(int result) {
  HandleWriteResult(result);
}

void DatagramConnectionTester::HandleWriteResult(int result) {
  if (result <= 0 && result != net::ERR_IO_PENDING) {
    LOG(ERROR) << "Received error " << result << " when trying to write";
    write_errors_++;
    Done();
  } else if (result > 0) {
    EXPECT_EQ(message_size_, result);
    packets_sent_++;
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&DatagramConnectionTester::DoWrite, base::Unretained(this)),
        base::TimeDelta::FromMilliseconds(delay_ms_));
  }
}

void DatagramConnectionTester::DoRead() {
  int result = 1;
  while (result > 0) {
    int kReadSize = message_size_ * 2;
    read_buffer_ = new net::IOBuffer(kReadSize);

    result = host_socket_->Recv(
        read_buffer_.get(), kReadSize,
        base::Bind(&DatagramConnectionTester::OnRead, base::Unretained(this)));
    HandleReadResult(result);
  };
}

void DatagramConnectionTester::OnRead(int result) {
  HandleReadResult(result);
  DoRead();
}

void DatagramConnectionTester::HandleReadResult(int result) {
  if (result <= 0 && result != net::ERR_IO_PENDING) {
    // Error will be received after the socket is closed.
    LOG(ERROR) << "Received error " << result << " when trying to read";
    read_errors_++;
    Done();
  } else if (result > 0) {
    packets_received_++;
    if (message_size_ != result) {
      // Invalid packet size;
      bad_packets_received_++;
    } else {
      // Validate packet body.
      int packet_id;
      memcpy(&packet_id, read_buffer_->data(), sizeof(packet_id));
      if (packet_id < 0 || packet_id >= message_count_) {
        bad_packets_received_++;
      } else {
        if (memcmp(read_buffer_->data(), sent_packets_[packet_id]->data(),
                   message_size_) != 0)
          bad_packets_received_++;
      }
    }
  }
}

class MessagePipeConnectionTester::MessageSender
    : public MessagePipe::EventHandler {
 public:
  MessageSender(MessagePipe* pipe, int message_size, int message_count)
      : pipe_(pipe),
        message_size_(message_size),
        message_count_(message_count) {}

  void Start() { pipe_->Start(this); }

  const std::vector<std::unique_ptr<VideoPacket>>& sent_messages() {
    return sent_messages_;
  }

  // MessagePipe::EventHandler interface.
  void OnMessagePipeOpen() override {
    for (int i = 0; i < message_count_; ++i) {
      std::unique_ptr<VideoPacket> message(new VideoPacket());
      message->mutable_data()->resize(message_size_);
      for (int p = 0; p < message_size_; ++p) {
        message->mutable_data()[0] = static_cast<char>(i + p);
      }
      pipe_->Send(message.get(), base::Closure());
      sent_messages_.push_back(std::move(message));
    }
  }
  void OnMessageReceived(std::unique_ptr<CompoundBuffer> message) override {
    NOTREACHED();
  }
  void OnMessagePipeClosed() override { NOTREACHED(); }

 private:
  MessagePipe* pipe_;
  int message_size_;
  int message_count_;

  std::vector<std::unique_ptr<VideoPacket>> sent_messages_;
};

MessagePipeConnectionTester::MessagePipeConnectionTester(
    MessagePipe* host_pipe,
    MessagePipe* client_pipe,
    int message_size,
    int message_count)
    : client_pipe_(client_pipe),
      sender_(new MessageSender(host_pipe, message_size, message_count)) {}

MessagePipeConnectionTester::~MessagePipeConnectionTester() = default;

void MessagePipeConnectionTester::RunAndCheckResults() {
  sender_->Start();
  client_pipe_->Start(this);

  run_loop_.Run();

  ASSERT_EQ(sender_->sent_messages().size(), received_messages_.size());
  for (size_t i = 0; i < sender_->sent_messages().size(); ++i) {
    EXPECT_TRUE(sender_->sent_messages()[i]->data() ==
                received_messages_[i]->data());
  }
}

void MessagePipeConnectionTester::OnMessagePipeOpen() {}

void MessagePipeConnectionTester::OnMessageReceived(
    std::unique_ptr<CompoundBuffer> message) {
  received_messages_.push_back(ParseMessage<VideoPacket>(message.get()));
  if (received_messages_.size() >= sender_->sent_messages().size()) {
    run_loop_.Quit();
  }
}

void MessagePipeConnectionTester::OnMessagePipeClosed() {
  run_loop_.Quit();
  FAIL();
}

}  // namespace protocol
}  // namespace remoting
