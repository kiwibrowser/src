// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_CONNECTION_TESTER_H_
#define REMOTING_PROTOCOL_CONNECTION_TESTER_H_

#include <list>
#include <memory>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "remoting/protocol/message_pipe.h"

namespace net {
class DrainableIOBuffer;
class GrowableIOBuffer;
class IOBuffer;
}  // namespace net

namespace remoting {

class CompoundBuffer;
class VideoPacket;

namespace protocol {

class P2PDatagramSocket;
class P2PStreamSocket;

// This class is used by unit tests to verify that a connection
// between two sockets works properly, i.e. data is delivered from one
// end to the other.
class StreamConnectionTester {
 public:
  StreamConnectionTester(P2PStreamSocket* client_socket,
                         P2PStreamSocket* host_socket,
                         int message_size,
                         int message_count);
  ~StreamConnectionTester();

  void Start();
  bool done() { return done_; }
  void CheckResults();

 protected:
  void Done();
  void InitBuffers();
  void DoWrite();
  void OnWritten(int result);
  void HandleWriteResult(int result);
  void DoRead();
  void OnRead(int result);
  void HandleReadResult(int result);

 private:
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  P2PStreamSocket* host_socket_;
  P2PStreamSocket* client_socket_;
  int message_size_;
  int test_data_size_;
  bool done_;

  scoped_refptr<net::DrainableIOBuffer> output_buffer_;
  scoped_refptr<net::GrowableIOBuffer> input_buffer_;

  int write_errors_;
  int read_errors_;
};

class DatagramConnectionTester {
 public:
  DatagramConnectionTester(P2PDatagramSocket* client_socket,
                           P2PDatagramSocket* host_socket,
                           int message_size,
                           int message_count,
                           int delay_ms);
  ~DatagramConnectionTester() ;

  void Start();
  void CheckResults();

 private:
  void Done();
  void DoWrite();
  void OnWritten(int result);
  void HandleWriteResult(int result);
  void DoRead();
  void OnRead(int result);
  void HandleReadResult(int result);

  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  P2PDatagramSocket* host_socket_;
  P2PDatagramSocket* client_socket_;
  int message_size_;
  int message_count_;
  int delay_ms_;
  bool done_;

  std::vector<scoped_refptr<net::IOBuffer> > sent_packets_;
  scoped_refptr<net::IOBuffer> read_buffer_;

  int write_errors_;
  int read_errors_;
  int packets_sent_;
  int packets_received_;
  int bad_packets_received_;
};

class MessagePipeConnectionTester : public MessagePipe::EventHandler {
 public:
  MessagePipeConnectionTester(MessagePipe* host_pipe,
                              MessagePipe* client_pipe,
                              int message_size,
                              int message_count);
  ~MessagePipeConnectionTester() override;

  void RunAndCheckResults();

 protected:
  // MessagePipe::EventHandler interface.
  void OnMessagePipeOpen() override;
  void OnMessageReceived(std::unique_ptr<CompoundBuffer> message) override;
  void OnMessagePipeClosed() override;

 private:
  class MessageSender;

  base::RunLoop run_loop_;
  MessagePipe* client_pipe_;

  std::unique_ptr<MessageSender> sender_;

  std::vector<std::unique_ptr<VideoPacket>> received_messages_;
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_CONNECTION_TESTER_H_
