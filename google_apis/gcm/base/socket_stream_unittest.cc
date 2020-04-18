// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gcm/base/socket_stream.h"

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_piece.h"
#include "net/base/ip_address.h"
#include "net/log/net_log_source.h"
#include "net/socket/socket_test_util.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gcm {
namespace {

typedef std::vector<net::MockRead> ReadList;
typedef std::vector<net::MockWrite> WriteList;

const char kReadData[] = "read_data";
const int kReadDataSize = arraysize(kReadData) - 1;
const char kReadData2[] = "read_alternate_data";
const int kReadData2Size = arraysize(kReadData2) - 1;
const char kWriteData[] = "write_data";
const int kWriteDataSize = arraysize(kWriteData) - 1;

class GCMSocketStreamTest : public testing::Test {
 public:
  GCMSocketStreamTest();
  ~GCMSocketStreamTest() override;

  // Build a socket with the expected reads and writes.
  void BuildSocket(const ReadList& read_list, const WriteList& write_list);

  // Pump the message loop until idle.
  void PumpLoop();

  // Simulates a google::protobuf::io::CodedInputStream read.
  base::StringPiece DoInputStreamRead(int bytes);
  // Simulates a google::protobuf::io::CodedOutputStream write.
  int DoOutputStreamWrite(const base::StringPiece& write_src);

  // Synchronous Refresh wrapper.
  void WaitForData(int msg_size);

  base::MessageLoop* message_loop() { return &message_loop_; };
  net::StaticSocketDataProvider* data_provider() {
    return data_provider_.get();
  }
  SocketInputStream* input_stream() { return socket_input_stream_.get(); }
  SocketOutputStream* output_stream() { return socket_output_stream_.get(); }
  net::StreamSocket* socket() { return socket_.get(); }

 private:
  void OpenConnection();
  void ResetInputStream();
  void ResetOutputStream();

  void ConnectCallback(int result);

  // SocketStreams and their data providers.
  ReadList mock_reads_;
  WriteList mock_writes_;
  std::unique_ptr<net::StaticSocketDataProvider> data_provider_;
  std::unique_ptr<SocketInputStream> socket_input_stream_;
  std::unique_ptr<SocketOutputStream> socket_output_stream_;

  // net:: components.
  std::unique_ptr<net::StreamSocket> socket_;
  net::MockClientSocketFactory socket_factory_;
  net::AddressList address_list_;

  base::MessageLoopForIO message_loop_;
};

GCMSocketStreamTest::GCMSocketStreamTest() {
  address_list_ = net::AddressList::CreateFromIPAddress(
      net::IPAddress::IPv4Localhost(), 5228);
}

GCMSocketStreamTest::~GCMSocketStreamTest() {}

void GCMSocketStreamTest::BuildSocket(const ReadList& read_list,
                                      const WriteList& write_list) {
  mock_reads_ = read_list;
  mock_writes_ = write_list;
  data_provider_.reset(
      new net::StaticSocketDataProvider(mock_reads_, mock_writes_));
  socket_factory_.AddSocketDataProvider(data_provider_.get());
  OpenConnection();
  ResetInputStream();
  ResetOutputStream();
}

void GCMSocketStreamTest::PumpLoop() {
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();
}

base::StringPiece GCMSocketStreamTest::DoInputStreamRead(int bytes) {
  int total_bytes_read = 0;
  const void* initial_buffer = NULL;
  const void* buffer = NULL;
  int size = 0;

  do {
    DCHECK(socket_input_stream_->GetState() == SocketInputStream::EMPTY ||
           socket_input_stream_->GetState() == SocketInputStream::READY);
    if (!socket_input_stream_->Next(&buffer, &size))
      break;
    total_bytes_read += size;
    if (initial_buffer) {  // Verify the buffer doesn't skip data.
      EXPECT_EQ(static_cast<const uint8_t*>(initial_buffer) + total_bytes_read,
                static_cast<const uint8_t*>(buffer) + size);
    } else {
      initial_buffer = buffer;
    }
  } while (total_bytes_read < bytes);

  if (total_bytes_read > bytes) {
    socket_input_stream_->BackUp(total_bytes_read - bytes);
    total_bytes_read = bytes;
  }

  return base::StringPiece(static_cast<const char*>(initial_buffer),
                           total_bytes_read);
}

int GCMSocketStreamTest::DoOutputStreamWrite(
    const base::StringPiece& write_src) {
  DCHECK_EQ(socket_output_stream_->GetState(), SocketOutputStream::EMPTY);
  int total_bytes_written = 0;
  void* buffer = NULL;
  int size = 0;
  int bytes = write_src.size();

  do {
    if (!socket_output_stream_->Next(&buffer, &size))
      break;
    int bytes_to_write = (size < bytes ? size : bytes);
    memcpy(buffer,
           write_src.data() + total_bytes_written,
           bytes_to_write);
    if (bytes_to_write < size)
      socket_output_stream_->BackUp(size - bytes_to_write);
    total_bytes_written += bytes_to_write;
  } while (total_bytes_written < bytes);

  base::RunLoop run_loop;
  if (socket_output_stream_->Flush(run_loop.QuitClosure()) ==
          net::ERR_IO_PENDING) {
    run_loop.Run();
  }

  return total_bytes_written;
}

void GCMSocketStreamTest::WaitForData(int msg_size) {
  while (input_stream()->UnreadByteCount() < msg_size) {
    base::RunLoop run_loop;
    if (input_stream()->Refresh(run_loop.QuitClosure(),
                                msg_size - input_stream()->UnreadByteCount()) ==
            net::ERR_IO_PENDING) {
      run_loop.Run();
    }
    if (input_stream()->GetState() == SocketInputStream::CLOSED)
      return;
  }
}

void GCMSocketStreamTest::OpenConnection() {
  socket_ = socket_factory_.CreateTransportClientSocket(
      address_list_, NULL, NULL, net::NetLogSource());
  socket_->Connect(
      base::Bind(&GCMSocketStreamTest::ConnectCallback,
                 base::Unretained(this)));
  PumpLoop();
}

void GCMSocketStreamTest::ConnectCallback(int result) {}

void GCMSocketStreamTest::ResetInputStream() {
  DCHECK(socket_.get());
  socket_input_stream_.reset(new SocketInputStream(socket_.get()));
}

void GCMSocketStreamTest::ResetOutputStream() {
  DCHECK(socket_.get());
  socket_output_stream_.reset(
      new SocketOutputStream(socket_.get(), TRAFFIC_ANNOTATION_FOR_TESTS));
}

// A read where all data is already available.
TEST_F(GCMSocketStreamTest, ReadDataSync) {
  BuildSocket(ReadList(1, net::MockRead(net::SYNCHRONOUS,
                                        kReadData,
                                        kReadDataSize)),
              WriteList());

  WaitForData(kReadDataSize);
  ASSERT_EQ(std::string(kReadData, kReadDataSize),
            DoInputStreamRead(kReadDataSize));
}

// A read that comes in two parts.
TEST_F(GCMSocketStreamTest, ReadPartialDataSync) {
  int first_read_len = kReadDataSize / 2;
  int second_read_len = kReadDataSize - first_read_len;
  ReadList read_list;
  read_list.push_back(
      net::MockRead(net::SYNCHRONOUS,
                    kReadData,
                    first_read_len));
  read_list.push_back(
      net::MockRead(net::SYNCHRONOUS,
                    &kReadData[first_read_len],
                    second_read_len));
  BuildSocket(read_list, WriteList());

  WaitForData(kReadDataSize);
  ASSERT_EQ(std::string(kReadData, kReadDataSize),
            DoInputStreamRead(kReadDataSize));
}

// A read where no data is available at first (IO_PENDING will be returned).
TEST_F(GCMSocketStreamTest, ReadAsync) {
  int first_read_len = kReadDataSize / 2;
  int second_read_len = kReadDataSize - first_read_len;
  ReadList read_list;
  read_list.push_back(
      net::MockRead(net::ASYNC, kReadData, first_read_len));
  read_list.push_back(
      net::MockRead(net::ASYNC, &kReadData[first_read_len], second_read_len));
  BuildSocket(read_list, WriteList());
  WaitForData(kReadDataSize);
  ASSERT_EQ(std::string(kReadData, kReadDataSize),
            DoInputStreamRead(kReadDataSize));
}

// Simulate two packets arriving at once. Read them in two separate calls.
TEST_F(GCMSocketStreamTest, TwoReadsAtOnce) {
  std::string long_data = std::string(kReadData, kReadDataSize) +
                          std::string(kReadData2, kReadData2Size);
  BuildSocket(ReadList(1, net::MockRead(net::SYNCHRONOUS,
                                        long_data.c_str(),
                                        long_data.size())),
              WriteList());

  WaitForData(kReadDataSize);
  ASSERT_EQ(std::string(kReadData, kReadDataSize),
            DoInputStreamRead(kReadDataSize));

  WaitForData(kReadData2Size);
  ASSERT_EQ(std::string(kReadData2, kReadData2Size),
            DoInputStreamRead(kReadData2Size));
}

// Simulate two packets arriving at once. Read them in two calls separated
// by a Rebuild.
TEST_F(GCMSocketStreamTest, TwoReadsAtOnceWithRebuild) {
  std::string long_data = std::string(kReadData, kReadDataSize) +
                          std::string(kReadData2, kReadData2Size);
  BuildSocket(ReadList(1, net::MockRead(net::SYNCHRONOUS,
                                        long_data.c_str(),
                                        long_data.size())),
              WriteList());

  WaitForData(kReadDataSize);
  ASSERT_EQ(std::string(kReadData, kReadDataSize),
              DoInputStreamRead(kReadDataSize));

  input_stream()->RebuildBuffer();
  WaitForData(kReadData2Size);
  ASSERT_EQ(std::string(kReadData2, kReadData2Size),
            DoInputStreamRead(kReadData2Size));
}

// Simulate a read that is aborted.
TEST_F(GCMSocketStreamTest, ReadError) {
  int result = net::ERR_ABORTED;
  BuildSocket(ReadList(1, net::MockRead(net::SYNCHRONOUS, result)),
              WriteList());

  WaitForData(kReadDataSize);
  ASSERT_EQ(SocketInputStream::CLOSED, input_stream()->GetState());
  ASSERT_EQ(result, input_stream()->last_error());
}

// Simulate a read after the connection is closed.
TEST_F(GCMSocketStreamTest, ReadDisconnected) {
  BuildSocket(ReadList(), WriteList());
  socket()->Disconnect();
  WaitForData(kReadDataSize);
  ASSERT_EQ(SocketInputStream::CLOSED, input_stream()->GetState());
  ASSERT_EQ(net::ERR_CONNECTION_CLOSED, input_stream()->last_error());
}

// Write a full message in one go.
TEST_F(GCMSocketStreamTest, WriteFull) {
  BuildSocket(ReadList(),
              WriteList(1, net::MockWrite(net::SYNCHRONOUS,
                                          kWriteData,
                                          kWriteDataSize)));
  ASSERT_EQ(kWriteDataSize,
            DoOutputStreamWrite(base::StringPiece(kWriteData,
                                                  kWriteDataSize)));
}

// Write a message in two go's.
TEST_F(GCMSocketStreamTest, WritePartial) {
  WriteList write_list;
  write_list.push_back(net::MockWrite(net::SYNCHRONOUS,
                                      kWriteData,
                                      kWriteDataSize / 2));
  write_list.push_back(net::MockWrite(net::SYNCHRONOUS,
                                      kWriteData + kWriteDataSize / 2,
                                      kWriteDataSize / 2));
  BuildSocket(ReadList(), write_list);
  ASSERT_EQ(kWriteDataSize,
            DoOutputStreamWrite(base::StringPiece(kWriteData,
                                                  kWriteDataSize)));
}

// Write a message completely asynchronously (returns IO_PENDING before
// finishing the write in two go's).
TEST_F(GCMSocketStreamTest, WriteNone) {
  WriteList write_list;
  write_list.push_back(net::MockWrite(net::SYNCHRONOUS,
                                      kWriteData,
                                      kWriteDataSize / 2));
  write_list.push_back(net::MockWrite(net::SYNCHRONOUS,
                                      kWriteData + kWriteDataSize / 2,
                                      kWriteDataSize / 2));
  BuildSocket(ReadList(), write_list);
  ASSERT_EQ(kWriteDataSize,
            DoOutputStreamWrite(base::StringPiece(kWriteData,
                                                  kWriteDataSize)));
}

// Write a message then read a message.
TEST_F(GCMSocketStreamTest, WriteThenRead) {
  BuildSocket(ReadList(1, net::MockRead(net::SYNCHRONOUS,
                                        kReadData,
                                        kReadDataSize)),
              WriteList(1, net::MockWrite(net::SYNCHRONOUS,
                                          kWriteData,
                                          kWriteDataSize)));

  ASSERT_EQ(kWriteDataSize,
            DoOutputStreamWrite(base::StringPiece(kWriteData,
                                                  kWriteDataSize)));

  WaitForData(kReadDataSize);
  ASSERT_EQ(std::string(kReadData, kReadDataSize),
              DoInputStreamRead(kReadDataSize));
}

// Read a message then write a message.
TEST_F(GCMSocketStreamTest, ReadThenWrite) {
  BuildSocket(ReadList(1, net::MockRead(net::SYNCHRONOUS,
                                        kReadData,
                                        kReadDataSize)),
              WriteList(1, net::MockWrite(net::SYNCHRONOUS,
                                          kWriteData,
                                          kWriteDataSize)));

  WaitForData(kReadDataSize);
  ASSERT_EQ(std::string(kReadData, kReadDataSize),
              DoInputStreamRead(kReadDataSize));

  ASSERT_EQ(kWriteDataSize,
            DoOutputStreamWrite(base::StringPiece(kWriteData,
                                                  kWriteDataSize)));
}

// Simulate a write that gets aborted.
TEST_F(GCMSocketStreamTest, WriteError) {
  int result = net::ERR_ABORTED;
  BuildSocket(ReadList(),
              WriteList(1, net::MockWrite(net::SYNCHRONOUS, result)));
  DoOutputStreamWrite(base::StringPiece(kWriteData, kWriteDataSize));
  ASSERT_EQ(SocketOutputStream::CLOSED, output_stream()->GetState());
  ASSERT_EQ(result, output_stream()->last_error());
}

// Simulate a write after the connection is closed.
TEST_F(GCMSocketStreamTest, WriteDisconnected) {
  BuildSocket(ReadList(), WriteList());
  socket()->Disconnect();
  DoOutputStreamWrite(base::StringPiece(kWriteData, kWriteDataSize));
  ASSERT_EQ(SocketOutputStream::CLOSED, output_stream()->GetState());
  ASSERT_EQ(net::ERR_CONNECTION_CLOSED, output_stream()->last_error());
}

}  // namespace
}  // namespace gcm
