// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/test/mojo_test_base.h"

#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/system/handle_signals_state.h"
#include "mojo/public/c/system/buffer.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/c/system/functions.h"
#include "mojo/public/c/system/trap.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/wait.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_MACOSX) && !defined(OS_IOS)
#include "base/mac/mach_port_broker.h"
#endif

namespace mojo {
namespace edk {
namespace test {

#if defined(OS_MACOSX) && !defined(OS_IOS)
namespace {
base::MachPortBroker* g_mach_broker = nullptr;
}
#endif

MojoTestBase::MojoTestBase() {
#if defined(OS_MACOSX) && !defined(OS_IOS)
  if (!g_mach_broker) {
    g_mach_broker = new base::MachPortBroker("mojo_test");
    CHECK(g_mach_broker->Init());
    SetMachPortProvider(g_mach_broker);
  }
#endif
}

MojoTestBase::~MojoTestBase() {}

MojoTestBase::ClientController& MojoTestBase::StartClient(
    const std::string& client_name) {
  clients_.push_back(std::make_unique<ClientController>(
      client_name, this, process_error_callback_, launch_type_));
  return *clients_.back();
}

MojoTestBase::ClientController::ClientController(
    const std::string& client_name,
    MojoTestBase* test,
    const ProcessErrorCallback& process_error_callback,
    LaunchType launch_type) {
#if !defined(OS_IOS)
#if defined(OS_MACOSX)
  // This lock needs to be held while launching the child because the Mach port
  // broker only allows task ports to be received from known child processes.
  // However, it can only know the child process's pid after the child has
  // launched. To prevent a race where the child process sends its task port
  // before the pid has been registered, the lock needs to be held over both
  // launch and child pid registration.
  base::AutoLock lock(g_mach_broker->GetLock());
#endif
  helper_.set_process_error_callback(process_error_callback);
  pipe_ = helper_.StartChild(client_name, launch_type);
#if defined(OS_MACOSX)
  g_mach_broker->AddPlaceholderForPid(helper_.test_child().Handle());
#endif
#endif
}

MojoTestBase::ClientController::~ClientController() {
  CHECK(was_shutdown_)
      << "Test clients should be waited on explicitly with WaitForShutdown().";
}

void MojoTestBase::ClientController::ClosePeerConnection() {
#if !defined(OS_IOS)
  helper_.ClosePeerConnection();
#endif
}

int MojoTestBase::ClientController::WaitForShutdown() {
  was_shutdown_ = true;
#if !defined(OS_IOS)
  int retval = helper_.WaitForChildShutdown();
#if defined(OS_MACOSX)
  base::AutoLock lock(g_mach_broker->GetLock());
  g_mach_broker->InvalidatePid(helper_.test_child().Handle());
#endif
  return retval;
#else
  NOTREACHED();
  return 1;
#endif
}

// static
void MojoTestBase::CloseHandle(MojoHandle h) {
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h));
}

// static
void MojoTestBase::CreateMessagePipe(MojoHandle* p0, MojoHandle* p1) {
  MojoCreateMessagePipe(nullptr, p0, p1);
  CHECK_NE(*p0, MOJO_HANDLE_INVALID);
  CHECK_NE(*p1, MOJO_HANDLE_INVALID);
}

// static
void MojoTestBase::WriteMessageWithHandles(MojoHandle mp,
                                           const std::string& message,
                                           const MojoHandle* handles,
                                           uint32_t num_handles) {
  CHECK_EQ(WriteMessageRaw(MessagePipeHandle(mp), message.data(),
                           static_cast<uint32_t>(message.size()), handles,
                           num_handles, MOJO_WRITE_MESSAGE_FLAG_NONE),
           MOJO_RESULT_OK);
}

// static
void MojoTestBase::WriteMessage(MojoHandle mp, const std::string& message) {
  WriteMessageWithHandles(mp, message, nullptr, 0);
}

// static
std::string MojoTestBase::ReadMessageWithHandles(
    MojoHandle mp,
    MojoHandle* out_handles,
    uint32_t expected_num_handles) {
  CHECK_EQ(WaitForSignals(mp, MOJO_HANDLE_SIGNAL_READABLE), MOJO_RESULT_OK);

  std::vector<uint8_t> bytes;
  std::vector<ScopedHandle> handles;
  CHECK_EQ(MOJO_RESULT_OK,
           ReadMessageRaw(MessagePipeHandle(mp), &bytes, &handles,
                          MOJO_READ_MESSAGE_FLAG_NONE));
  CHECK_EQ(expected_num_handles, handles.size());
  for (size_t i = 0; i < handles.size(); ++i)
    out_handles[i] = handles[i].release().value();

  return std::string(bytes.begin(), bytes.end());
}

// static
std::string MojoTestBase::ReadMessageWithOptionalHandle(MojoHandle mp,
                                                        MojoHandle* handle) {
  CHECK_EQ(WaitForSignals(mp, MOJO_HANDLE_SIGNAL_READABLE), MOJO_RESULT_OK);

  std::vector<uint8_t> bytes;
  std::vector<ScopedHandle> handles;
  CHECK_EQ(MOJO_RESULT_OK,
           ReadMessageRaw(MessagePipeHandle(mp), &bytes, &handles,
                          MOJO_READ_MESSAGE_FLAG_NONE));
  CHECK(handles.size() == 0 || handles.size() == 1);
  CHECK(handle);

  if (handles.size() == 1)
    *handle = handles[0].release().value();
  else
    *handle = MOJO_HANDLE_INVALID;

  return std::string(bytes.begin(), bytes.end());
}

// static
std::string MojoTestBase::ReadMessage(MojoHandle mp) {
  return ReadMessageWithHandles(mp, nullptr, 0);
}

// static
void MojoTestBase::ReadMessage(MojoHandle mp, char* data, size_t num_bytes) {
  CHECK_EQ(WaitForSignals(mp, MOJO_HANDLE_SIGNAL_READABLE), MOJO_RESULT_OK);

  std::vector<uint8_t> bytes;
  std::vector<ScopedHandle> handles;
  CHECK_EQ(MOJO_RESULT_OK,
           ReadMessageRaw(MessagePipeHandle(mp), &bytes, &handles,
                          MOJO_READ_MESSAGE_FLAG_NONE));
  CHECK_EQ(0u, handles.size());
  CHECK_EQ(num_bytes, bytes.size());
  memcpy(data, bytes.data(), bytes.size());
}

// static
void MojoTestBase::VerifyTransmission(MojoHandle source,
                                      MojoHandle dest,
                                      const std::string& message) {
  WriteMessage(source, message);

  // We don't use EXPECT_EQ; failures on really long messages make life hard.
  EXPECT_TRUE(message == ReadMessage(dest));
}

// static
void MojoTestBase::VerifyEcho(MojoHandle mp, const std::string& message) {
  VerifyTransmission(mp, mp, message);
}

// static
MojoHandle MojoTestBase::CreateBuffer(uint64_t size) {
  MojoHandle h;
  EXPECT_EQ(MojoCreateSharedBuffer(size, nullptr, &h), MOJO_RESULT_OK);
  return h;
}

// static
MojoHandle MojoTestBase::DuplicateBuffer(MojoHandle h, bool read_only) {
  MojoHandle new_handle;
  MojoDuplicateBufferHandleOptions options = {
      sizeof(MojoDuplicateBufferHandleOptions),
      MOJO_DUPLICATE_BUFFER_HANDLE_FLAG_NONE};
  if (read_only)
    options.flags |= MOJO_DUPLICATE_BUFFER_HANDLE_FLAG_READ_ONLY;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoDuplicateBufferHandle(h, &options, &new_handle));
  return new_handle;
}

// static
void MojoTestBase::WriteToBuffer(MojoHandle h,
                                 size_t offset,
                                 const base::StringPiece& s) {
  char* data;
  EXPECT_EQ(MOJO_RESULT_OK, MojoMapBuffer(h, offset, s.size(), nullptr,
                                          reinterpret_cast<void**>(&data)));
  memcpy(data, s.data(), s.size());
  EXPECT_EQ(MOJO_RESULT_OK, MojoUnmapBuffer(static_cast<void*>(data)));
}

// static
void MojoTestBase::ExpectBufferContents(MojoHandle h,
                                        size_t offset,
                                        const base::StringPiece& s) {
  char* data;
  EXPECT_EQ(MOJO_RESULT_OK, MojoMapBuffer(h, offset, s.size(), nullptr,
                                          reinterpret_cast<void**>(&data)));
  EXPECT_EQ(s, base::StringPiece(data, s.size()));
  EXPECT_EQ(MOJO_RESULT_OK, MojoUnmapBuffer(static_cast<void*>(data)));
}

// static
void MojoTestBase::CreateDataPipe(MojoHandle* p0,
                                  MojoHandle* p1,
                                  size_t capacity) {
  MojoCreateDataPipeOptions options;
  options.struct_size = static_cast<uint32_t>(sizeof(options));
  options.flags = MOJO_CREATE_DATA_PIPE_FLAG_NONE;
  options.element_num_bytes = 1;
  options.capacity_num_bytes = static_cast<uint32_t>(capacity);

  MojoCreateDataPipe(&options, p0, p1);
  CHECK_NE(*p0, MOJO_HANDLE_INVALID);
  CHECK_NE(*p1, MOJO_HANDLE_INVALID);
}

// static
void MojoTestBase::WriteData(MojoHandle producer, const std::string& data) {
  CHECK_EQ(WaitForSignals(producer, MOJO_HANDLE_SIGNAL_WRITABLE),
           MOJO_RESULT_OK);
  uint32_t num_bytes = static_cast<uint32_t>(data.size());
  MojoWriteDataOptions options;
  options.struct_size = sizeof(options);
  options.flags = MOJO_WRITE_DATA_FLAG_ALL_OR_NONE;
  CHECK_EQ(MojoWriteData(producer, data.data(), &num_bytes, &options),
           MOJO_RESULT_OK);
  CHECK_EQ(num_bytes, static_cast<uint32_t>(data.size()));
}

// static
std::string MojoTestBase::ReadData(MojoHandle consumer, size_t size) {
  CHECK_EQ(WaitForSignals(consumer, MOJO_HANDLE_SIGNAL_READABLE),
           MOJO_RESULT_OK);
  std::vector<char> buffer(size);
  uint32_t num_bytes = static_cast<uint32_t>(size);
  MojoReadDataOptions options;
  options.struct_size = sizeof(options);
  options.flags = MOJO_READ_DATA_FLAG_ALL_OR_NONE;
  CHECK_EQ(MojoReadData(consumer, &options, buffer.data(), &num_bytes),
           MOJO_RESULT_OK);
  CHECK_EQ(num_bytes, static_cast<uint32_t>(size));

  return std::string(buffer.begin(), buffer.end());
}

// static
MojoHandleSignalsState MojoTestBase::GetSignalsState(MojoHandle handle) {
  MojoHandleSignalsState signals_state;
  CHECK_EQ(MOJO_RESULT_OK, MojoQueryHandleSignalsState(handle, &signals_state));
  return signals_state;
}

// static
MojoResult MojoTestBase::WaitForSignals(MojoHandle handle,
                                        MojoHandleSignals signals,
                                        MojoTriggerCondition condition,
                                        MojoHandleSignalsState* state) {
  return Wait(Handle(handle), signals, condition, state);
}

// static
MojoResult MojoTestBase::WaitForSignals(MojoHandle handle,
                                        MojoHandleSignals signals,
                                        MojoHandleSignalsState* state) {
  return Wait(Handle(handle), signals, MOJO_TRIGGER_CONDITION_SIGNALS_SATISFIED,
              state);
}

}  // namespace test
}  // namespace edk
}  // namespace mojo
