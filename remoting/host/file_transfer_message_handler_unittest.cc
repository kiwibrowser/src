// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_transfer_message_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/containers/queue.h"
#include "base/memory/ptr_util.h"
#include "net/base/io_buffer.h"
#include "remoting/base/compound_buffer.h"
#include "remoting/host/file_proxy_wrapper.h"
#include "remoting/protocol/fake_message_pipe.h"
#include "remoting/protocol/fake_message_pipe_wrapper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kTestDatachannelName[] = "filetransfer-test";
constexpr char kTestFilename[] = "test-file.txt";

std::unique_ptr<remoting::CompoundBuffer> ToBuffer(const std::string& data) {
  std::unique_ptr<remoting::CompoundBuffer> buffer =
      std::make_unique<remoting::CompoundBuffer>();
  buffer->Append(new net::WrappedIOBuffer(data.data()), data.size());
  return buffer;
}

// base::queue doesn't provide operator==.
template <typename T>
bool QueuesEqual(const base::queue<T>& a, const base::queue<T>& b) {
  if (a.size() != b.size())
    return false;

  auto a_copy = a;
  auto b_copy = b;
  while (!a_copy.empty()) {
    if (a_copy.front() != b_copy.front())
      return false;
    a_copy.pop();
    b_copy.pop();
  }
  return true;
}

}  // namespace

namespace remoting {

class FakeFileProxyWrapper : public FileProxyWrapper {
 public:
  FakeFileProxyWrapper();
  ~FakeFileProxyWrapper() override;

  // FileProxyWrapper implementation.
  void Init(StatusCallback status_callback) override;
  void CreateFile(const base::FilePath& directory,
                  const std::string& filename) override;
  void OpenFile(const base::FilePath& filepath,
                OpenFileCallback open_callback) override;
  void WriteChunk(std::unique_ptr<CompoundBuffer> buffer) override;
  void ReadChunk(uint64_t chunk_size, ReadCallback read_callback) override;
  void Close() override;
  void Cancel() override;
  State state() override;

  void RunStatusCallback(
      base::Optional<protocol::FileTransferResponse_ErrorCode> error);
  const std::string& filename();
  base::queue<std::vector<char>> chunks();

 private:
  State state_ = kUninitialized;
  StatusCallback status_callback_;
  std::string filename_;
  base::queue<std::vector<char>> chunks_;
};

class FileTransferMessageHandlerTest : public testing::Test {
 public:
  FileTransferMessageHandlerTest();
  ~FileTransferMessageHandlerTest() override;

  // testing::Test implementation.
  void SetUp() override;
  void TearDown() override;

 protected:
  const std::string kTestDataOne = "this is the first test string";
  const std::string kTestDataTwo = "this is the second test string";

  std::unique_ptr<protocol::FakeMessagePipe> fake_pipe_;
  protocol::FileTransferRequest fake_request_;
  std::string fake_request_string_;
};

FakeFileProxyWrapper::FakeFileProxyWrapper() = default;
FakeFileProxyWrapper::~FakeFileProxyWrapper() = default;

void FakeFileProxyWrapper::Init(StatusCallback status_callback) {
  ASSERT_EQ(state_, kUninitialized);
  state_ = kInitialized;

  status_callback_ = std::move(status_callback);
}

void FakeFileProxyWrapper::CreateFile(const base::FilePath& directory,
                                      const std::string& filename) {
  ASSERT_EQ(state_, kInitialized);
  state_ = kReady;

  filename_ = filename;
}

void FakeFileProxyWrapper::OpenFile(const base::FilePath& filepath,
                                    OpenFileCallback open_callback) {
  ASSERT_EQ(state_, kInitialized);
  state_ = kReady;

  // TODO(jarhar): Implement fake file reading.
}

void FakeFileProxyWrapper::WriteChunk(std::unique_ptr<CompoundBuffer> buffer) {
  ASSERT_EQ(state_, kReady);

  std::vector<char> data;
  data.resize(buffer->total_bytes());
  buffer->CopyTo(data.data(), data.size());
  chunks_.push(data);
}

void FakeFileProxyWrapper::ReadChunk(uint64_t chunk_size,
                                     ReadCallback read_callback) {
  ASSERT_EQ(state_, kReady);

  // TODO(jarhar): Implement fake file reading.
}

void FakeFileProxyWrapper::Close() {
  ASSERT_EQ(state_, kReady);
  state_ = kClosed;
}

void FakeFileProxyWrapper::Cancel() {
  state_ = kFailed;
}

FileProxyWrapper::State FakeFileProxyWrapper::state() {
  return state_;
}

void FakeFileProxyWrapper::RunStatusCallback(
    base::Optional<protocol::FileTransferResponse_ErrorCode> error) {
  std::move(status_callback_).Run(state_, error);
}

const std::string& FakeFileProxyWrapper::filename() {
  return filename_;
}

base::queue<std::vector<char>> FakeFileProxyWrapper::chunks() {
  return chunks_;
}

FileTransferMessageHandlerTest::FileTransferMessageHandlerTest() = default;
FileTransferMessageHandlerTest::~FileTransferMessageHandlerTest() = default;

void FileTransferMessageHandlerTest::SetUp() {
  fake_pipe_ =
      base::WrapUnique(new protocol::FakeMessagePipe(false /* asynchronous */));

  fake_request_ = protocol::FileTransferRequest();
  fake_request_.set_filename(kTestFilename);
  fake_request_.set_filesize(kTestDataOne.size() + kTestDataTwo.size());
  fake_request_.SerializeToString(&fake_request_string_);
}

void FileTransferMessageHandlerTest::TearDown() {}

// Verify that the message handler creates, writes to, and closes a
// FileProxyWrapper without errors when given valid input.
TEST_F(FileTransferMessageHandlerTest, WriteTwoChunks) {
  std::unique_ptr<FakeFileProxyWrapper> file_proxy_wrapper =
      std::make_unique<FakeFileProxyWrapper>();
  // |file_proxy_wrapper_ptr| is valid until fake_pipe_->ClosePipe() is called.
  FakeFileProxyWrapper* file_proxy_wrapper_ptr = file_proxy_wrapper.get();

  // This will delete itself when fake_pipe_->ClosePipe() is called.
  new FileTransferMessageHandler(kTestDatachannelName, fake_pipe_->Wrap(),
                                 std::move(file_proxy_wrapper));

  fake_pipe_->OpenPipe();
  fake_pipe_->Receive(ToBuffer(fake_request_string_));
  fake_pipe_->Receive(ToBuffer(kTestDataOne));
  fake_pipe_->Receive(ToBuffer(kTestDataTwo));

  file_proxy_wrapper_ptr->RunStatusCallback(
      base::Optional<protocol::FileTransferResponse_ErrorCode>());

  base::queue<std::vector<char>> actual_chunks =
      file_proxy_wrapper_ptr->chunks();

  fake_pipe_->ClosePipe();
  file_proxy_wrapper_ptr = nullptr;

  base::queue<std::vector<char>> expected_chunks;
  expected_chunks.push(
      std::vector<char>(kTestDataOne.begin(), kTestDataOne.end()));
  expected_chunks.push(
      std::vector<char>(kTestDataTwo.begin(), kTestDataTwo.end()));
  ASSERT_TRUE(QueuesEqual(expected_chunks, actual_chunks));

  const base::queue<std::string>& actual_sent_messages =
      fake_pipe_->sent_messages();
  protocol::FileTransferResponse expected_response;
  expected_response.set_state(
      protocol::FileTransferResponse_TransferState_DONE);
  expected_response.set_total_bytes_written(fake_request_.filesize());
  std::string expected_response_string;
  expected_response.SerializeToString(&expected_response_string);
  base::queue<std::string> expected_sent_messages;
  expected_sent_messages.push(expected_response_string);
  ASSERT_TRUE(QueuesEqual(expected_sent_messages, actual_sent_messages));
}

// Verifies that the message handler sends an error protobuf when
// FileProxyWrapper returns an error.
TEST_F(FileTransferMessageHandlerTest, FileProxyError) {
  std::unique_ptr<FakeFileProxyWrapper> file_proxy_wrapper =
      std::make_unique<FakeFileProxyWrapper>();
  // |file_proxy_wrapper_ptr| is valid until fake_pipe_->ClosePipe() is called.
  FakeFileProxyWrapper* file_proxy_wrapper_ptr = file_proxy_wrapper.get();

  protocol::FileTransferResponse_ErrorCode fake_error =
      protocol::FileTransferResponse_ErrorCode_FILE_IO_ERROR;

  // This will delete itself when fake_pipe_->ClosePipe() is called.
  new FileTransferMessageHandler(kTestDatachannelName, fake_pipe_->Wrap(),
                                 std::move(file_proxy_wrapper));

  fake_pipe_->OpenPipe();
  fake_pipe_->Receive(ToBuffer(fake_request_string_));
  fake_pipe_->Receive(ToBuffer(kTestDataOne));

  file_proxy_wrapper_ptr->Cancel();
  file_proxy_wrapper_ptr->RunStatusCallback(
      base::Optional<protocol::FileTransferResponse_ErrorCode>(fake_error));

  fake_pipe_->ClosePipe();
  file_proxy_wrapper_ptr = nullptr;

  const base::queue<std::string>& actual_sent_messages =
      fake_pipe_->sent_messages();
  protocol::FileTransferResponse expected_response;
  expected_response.set_error(fake_error);
  std::string expected_response_string;
  expected_response.SerializeToString(&expected_response_string);
  base::queue<std::string> expected_sent_messages;
  expected_sent_messages.push(expected_response_string);
  ASSERT_TRUE(QueuesEqual(expected_sent_messages, actual_sent_messages));
}

}  // namespace remoting
