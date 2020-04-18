// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/public/cpp/sender_pipe.h"

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/services/heap_profiling/public/cpp/stream.h"

namespace heap_profiling {

namespace {

// The documentation for ::WriteFileEx indicates that the last parameter is an
// OVERLAPPED*. But OVERLAPPED has no member to hold a void* context for
// SenderPipe, and without that, the callback must only use global
// variables. This is problematic. The example
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365601(v=vs.85).aspx
// instead uses a struct whose first member is an OVERLAPPED object, and passes
// a struct pointer to ::WriteFileEx.
struct OverlappedWriteContext {
  OverlappedWriteContext()
      : waiting_for_write(true), bytes_written(0), error(ERROR_SUCCESS) {
    memset(&overlap, 0, sizeof(overlap));
    overlap.Offset = 0xFFFFFFFF;
    overlap.OffsetHigh = 0xFFFFFFFF;
  }

  // This must always be the first member.
  OVERLAPPED overlap;
  bool waiting_for_write;
  DWORD bytes_written;
  DWORD error;
};

static_assert(offsetof(OverlappedWriteContext, overlap) == 0,
              "overlap must always be the first member.");

// A global function called by ::WriteFileEx when the write has finished, or
// errored.
void WINAPI AsyncWriteFinishedGlobal(DWORD error,
                                     DWORD bytes_written,
                                     LPOVERLAPPED overlap) {
  OverlappedWriteContext* context =
      reinterpret_cast<OverlappedWriteContext*>(overlap);
  context->waiting_for_write = false;
  context->bytes_written = bytes_written;
  context->error = error;
}

}  // namespace

SenderPipe::PipePair::PipePair() {
  std::wstring pipe_name = base::StringPrintf(
      L"\\\\.\\pipe\\profiling.%u.%u.%I64u", GetCurrentProcessId(),
      GetCurrentThreadId(), base::RandUint64());

  HANDLE handle = CreateNamedPipe(
      pipe_name.c_str(),
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
      1,          // Max instances.
      kPipeSize,  // Out buffer size.
      kPipeSize,  // In buffer size.
      5000,  // Timeout in milliseconds for connecting the receiving pipe. Has
             // nothing to do with Send() timeout.
      nullptr);
  PCHECK(handle != INVALID_HANDLE_VALUE);
  receiver_.reset(mojo::edk::InternalPlatformHandle(handle));

  // Allow the handle to be inherited by child processes.
  SECURITY_ATTRIBUTES security_attributes;
  security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attributes.lpSecurityDescriptor = nullptr;
  security_attributes.bInheritHandle = TRUE;

  handle = CreateFile(
      pipe_name.c_str(), GENERIC_WRITE,
      0,  // No sharing.
      &security_attributes, OPEN_EXISTING,
      SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS | FILE_FLAG_OVERLAPPED,
      nullptr);
  PCHECK(handle != INVALID_HANDLE_VALUE);
  sender_.reset(mojo::edk::InternalPlatformHandle(handle));

  // Since a client has connected, ConnectNamedPipe() should return zero and
  // GetLastError() should return ERROR_PIPE_CONNECTED.
  BOOL result = ConnectNamedPipe(receiver_.get().handle, nullptr);
  DWORD error = GetLastError();
  CHECK((result == 0) && (error == ERROR_PIPE_CONNECTED));
}

SenderPipe::PipePair::PipePair(PipePair&& other) = default;

SenderPipe::SenderPipe(base::ScopedPlatformFile file)
    : file_(std::move(file)) {}

SenderPipe::~SenderPipe() {}

SenderPipe::Result SenderPipe::Send(const void* data,
                                    size_t size,
                                    int timeout_ms) {
  // The pipe is nonblocking. However, to ensure that messages on different
  // threads are serialized and in order:
  //   1) We grab a global lock.
  //   2) We attempt to synchronously write, but with a timeout. On timeout
  //   or error, the SenderPipe is shut down.
  base::AutoLock lock(lock_);

  // This can happen if Close() was called on another thread, while this thread
  // was already waiting to call SenderPipe::Send().
  if (!file_.IsValid())
    return Result::kError;

  // Queue an asynchronous write.
  OverlappedWriteContext context;

  // It's safe to use a raw pointer to |context|, since it will stay on the
  // stack until ::SleepEx returns, at which point either the callback has
  // finished, or will be cancelled.
  BOOL write_result = ::WriteFileEx(file_.Get(), data, static_cast<DWORD>(size),
                                    &context.overlap, AsyncWriteFinishedGlobal);

  // Check for errors.
  if (!write_result)
    return Result::kError;

  // The documentation for ::WriteFileEx
  // https://msdn.microsoft.com/en-us/library/windows/desktop/aa365748(v=vs.85).aspx
  // claims that we need to check GetLastError() even on success. This is
  // incorrect. GetLastError() returns the error from the previous Windows
  // library call.

  while (true) {
    // The return code of ::SleepEx has multiple semantics. Do not replace this
    // with PlatformThread::Sleep.
    DWORD sleep_result = ::SleepEx(timeout_ms, TRUE);

    // Timeout reached.
    if (sleep_result == 0) {
      BOOL r = ::CancelIo(file_.Get());
      DCHECK_NE(0, r);
      DWORD r2 = ::WaitForSingleObject(file_.Get(), INFINITE);
      DCHECK_EQ(WAIT_OBJECT_0, r2);
      return Result::kTimeout;
    }

    // Unexpected error.
    if (sleep_result != WAIT_IO_COMPLETION)
      return Result::kError;

    // In the very rare case where this function returns from the completion of
    // another async IO handler, just repeat the sleep duration. This allows us
    // to avoid a call to base::TimeTicks::Now() in the common case.
    if (context.waiting_for_write)
      continue;

    if (context.error != ERROR_SUCCESS)
      return Result::kError;

    // Partial writes should not be possible.
    DCHECK_EQ(context.bytes_written, size);
    return Result::kSuccess;
  }
}

void SenderPipe::Close() {
  base::AutoLock lock(lock_);
  file_.Close();
}

}  // namespace heap_profiling
