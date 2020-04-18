// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/driver.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/user.h>

#include "base/posix/eintr_wrapper.h"
#include "base/threading/thread_restrictions.h"
#include "chromeos/binder/binder_driver_api.h"

namespace binder {

namespace {

const char kDriverPath[] = "/dev/binder";

}  // namespace

Driver::Driver() : mmap_address_(MAP_FAILED) {}

Driver::~Driver() {
  base::AssertBlockingAllowed();
  if (mmap_address_ != MAP_FAILED) {
    if (munmap(mmap_address_, GetBinderMmapSize()) == -1) {
      PLOG(ERROR) << "Failed to munmap";
    }
  }
  fd_.reset();  // Close FD.
}

bool Driver::Initialize() {
  base::AssertBlockingAllowed();
  // Open binder driver.
  fd_.reset(HANDLE_EINTR(open(kDriverPath, O_RDWR | O_CLOEXEC | O_NONBLOCK)));
  if (!fd_.is_valid()) {
    PLOG(ERROR) << "Failed to open";
    return false;
  }
  // Version check.
  int version = 0;
  if (HANDLE_EINTR(ioctl(fd_.get(), BINDER_VERSION, &version)) != 0 ||
      version != BINDER_CURRENT_PROTOCOL_VERSION) {
    PLOG(ERROR) << "Version check failure: version = " << version;
    return false;
  }
  // Disable thread spawning.
  if (!SetMaxThreads(0)) {
    PLOG(ERROR) << "SetMaxThreads() failed";
    return false;
  }
  // Allocate buffer for transaction data.
  mmap_address_ = mmap(0, GetBinderMmapSize(), PROT_READ,
                       MAP_PRIVATE | MAP_NORESERVE, fd_.get(), 0);
  if (mmap_address_ == MAP_FAILED) {
    PLOG(ERROR) << "Failed to mmap";
    return false;
  }
  return true;
}

int Driver::GetFD() {
  return fd_.get();
}

bool Driver::SetMaxThreads(int max_threads) {
  base::AssertBlockingAllowed();
  return HANDLE_EINTR(ioctl(fd_.get(), BINDER_SET_MAX_THREADS, &max_threads)) !=
         -1;
}

bool Driver::WriteRead(const char* write_buf,
                       size_t write_buf_size,
                       char* read_buf,
                       size_t read_buf_size,
                       size_t* written_bytes,
                       size_t* read_bytes) {
  base::AssertBlockingAllowed();
  binder_write_read params = {};
  params.write_buffer = reinterpret_cast<const uintptr_t>(write_buf);
  params.write_size = write_buf_size;
  params.read_buffer = reinterpret_cast<uintptr_t>(read_buf);
  params.read_size = read_buf_size;
  if (HANDLE_EINTR(ioctl(fd_.get(), BINDER_WRITE_READ, &params)) < 0 &&
      errno != EAGAIN) {  // EAGAIN means there is no data to read.
    PLOG(ERROR) << "BINDER_WRITE_READ failed: write_buf_size = "
                << write_buf_size << ", read_buf_size = " << read_buf_size;
    return false;
  }
  *written_bytes = params.write_consumed;
  *read_bytes = params.read_consumed;
  return true;
}

bool Driver::Poll() {
  pollfd params = {};
  params.fd = fd_.get();
  params.events = POLLIN;
  const int kNumFds = 1;
  const int kTimeout = -1;  // No timeout.
  return HANDLE_EINTR(poll(&params, kNumFds, kTimeout)) == kNumFds;
}

bool Driver::NotifyCurrentThreadExiting() {
  base::AssertBlockingAllowed();
  return HANDLE_EINTR(ioctl(fd_.get(), BINDER_THREAD_EXIT, 0)) != -1;
}

size_t Driver::GetBinderMmapSize() const {
  // Subtract PAGESIZE * 2 to make room for guard pages. https://goo.gl/4Q6sPe
  return 1024 * 1024 - sysconf(_SC_PAGESIZE) * 2;
}

}  // namespace binder
