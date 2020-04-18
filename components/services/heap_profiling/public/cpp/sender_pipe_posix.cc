// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/public/cpp/sender_pipe.h"

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "components/services/heap_profiling/public/cpp/stream.h"

namespace heap_profiling {

SenderPipe::PipePair::PipePair() {
  // We create a pipe() rather than a socketpair(). On macOS, this causes writes
  // to be much more performant. On Linux, this causes slight improvements.
  // https://bugs.chromium.org/p/chromium/issues/detail?id=776435
  int fds[2];
  PCHECK(0 == pipe(fds));
  PCHECK(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0);
  PCHECK(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);
#if defined(OS_MACOSX)
  // On macOS, suppress SIGPIPE. On Linux, we must rely on the assumption that
  // the SIGPIPE signal is ignored [which it is].
  PCHECK(fcntl(fds[0], F_SETNOSIGPIPE, 1) == 0);
  PCHECK(fcntl(fds[1], F_SETNOSIGPIPE, 1) == 0);
#endif
  receiver_.reset(mojo::edk::InternalPlatformHandle(fds[0]));
  sender_.reset(mojo::edk::InternalPlatformHandle(fds[1]));
}

SenderPipe::PipePair::PipePair(PipePair&& other) = default;

SenderPipe::SenderPipe(base::ScopedPlatformFile file)
    : file_(std::move(file)) {}

SenderPipe::~SenderPipe() = default;

SenderPipe::Result SenderPipe::Send(const void* data,
                                    size_t sz,
                                    int timeout_ms) {
  base::AutoLock lock(lock_);

  // This can happen if Close() was called on another thread, while this thread
  // was already waiting to call SenderPipe::Send().
  if (!file_.is_valid())
    return Result::kError;

  int size = base::checked_cast<int>(sz);
  base::TimeTicks start_time;
  while (size > 0) {
    int r = HANDLE_EINTR(write(file_.get(), data, size));

    // On success!
    if (r != -1) {
      DCHECK_LE(r, size);
      size -= r;
      data = static_cast<const char*>(data) + r;
      continue;
    }

    // An error is either irrecoverable, or an I/O delay. Wait at most
    // timeout_ms seconds for the pipe to clear.
    int cached_errno = errno;
    if (cached_errno != EAGAIN && cached_errno != EWOULDBLOCK)
      return Result::kError;

    // Set the start time, if it hasn't already been set.
    base::TimeTicks now = base::TimeTicks::Now();
    if (start_time.is_null())
      start_time = now;

    // Calculate time left.
    int64_t time_left_ms =
        ((start_time + base::TimeDelta::FromMilliseconds(timeout_ms)) - now)
            .InMilliseconds();
    if (time_left_ms <= 0)
      return Result::kTimeout;

    // Wait for the pipe to be writeable.
    struct pollfd pfd = {file_.get(), POLLOUT, 0};
    int poll_result =
        HANDLE_EINTR(poll(&pfd, 1, static_cast<int>(time_left_ms)));
    if (poll_result == 0)
      return Result::kTimeout;
    if (poll_result == -1)
      return Result::kError;

    // If POLLOUT isn't returned, the pipe isn't writeable.
    DCHECK_EQ(poll_result, 1);
    if (!(pfd.revents & POLLOUT))
      return Result::kError;
  }
  return Result::kSuccess;
}

void SenderPipe::Close() {
  base::AutoLock lock(lock_);
  file_.reset();
}

}  // namespace heap_profiling
