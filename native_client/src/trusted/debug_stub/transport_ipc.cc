/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <string>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_scoped_ptr.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/debug_stub/transport.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


namespace port {

// This transport is to handle connections over ipc to another
// process. Instead of directly talking over a server socket,
// the server socket is listening on the other process and
// ferries the data between the socket and the given pipe.
//
// Since we also need to handle multiple sequential connections
// but cannot close our handle the incoming data is encoded.
// The encoding is simple and just prepends a 4 byte length
// to each message. A length of -1 signifies that the server
// socket was disconnected and is awaiting a new connection.
// Once in a disconnected state any new incoming data will
// symbolize a new connection.

// TODO(leslieb): implement for windows.
#if NACL_LINUX || NACL_OSX
class TransportIPC : public ITransport {
 public:
  TransportIPC()
    : buf_(new char[kBufSize]),
      unconsumed_bytes_(0),
      bytes_to_read_(kServerSocketDisconnect),
      handle_(NACL_INVALID_HANDLE) { }

  explicit TransportIPC(NaClHandle fd)
    : buf_(new char[kBufSize]),
      unconsumed_bytes_(0),
      bytes_to_read_(kServerSocketDisconnect),
      handle_(fd) { }

  ~TransportIPC() {
    if (handle_ != NACL_INVALID_HANDLE) {
      if (::close(handle_))
        NaClLog(LOG_FATAL,
                "TransportIPC::Disconnect: Failed to close handle.\n");
    }
  }

  // Read from this transport, return true on success.
  // Returning false means we have disconnected on the server end.
  virtual bool Read(void *ptr, int32_t len) {
    CHECK(IsConnected());
    CHECK(ptr && len >= 0);
    char *dst = static_cast<char *>(ptr);

    while (len > 0) {
      if (!FillBufferIfEmpty()) return false;
      CopyFromBuffer(&dst, &len);
    }
    return true;
  }

  // Write to this transport, return true on success.
  virtual bool Write(const void *ptr, int32_t len) {
    CHECK(IsConnected());
    CHECK(ptr && len >= 0);
    const char *src = static_cast<const char *>(ptr);
    while (len > 0) {
      int result = ::write(handle_, src, len);
      if (result > 0) {
        src += result;
        len -= result;
      } else if (result == 0 || errno != EINTR) {
        NaClLog(LOG_FATAL,
                "TransportIPC::Write: Pipe closed from other process.\n");
      }
    }
    return true;
  }

  // Return true if there is data to read.
  virtual bool IsDataAvailable() {
    CHECK(IsConnected());
    if (unconsumed_bytes_ > 0) return true;
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(handle_, &fds);

    // We want a "non-blocking" check
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Check if this file handle can select on read
    int cnt = select(handle_ + 1, &fds, 0, 0, &timeout);

    // If we are ready, or if there is an error.  We return true
    // on error, to let the next IO request fail.
    if (cnt != 0) return true;

    return false;
  }

  void WaitForDebugStubEvent(struct NaClApp *nap,
                             bool ignore_input_from_gdb) {
    bool wait = true;
    // If we are told to ignore messages from gdb, we will exit from this
    // function only if new data is sent by gdb.
    if ((unconsumed_bytes_ > 0 && !ignore_input_from_gdb) ||
        nap->faulted_thread_count > 0) {
      // Clear faulted thread events to save debug stub loop iterations.
      wait = false;
    }

    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(nap->faulted_thread_fd_read, &fds);
    int max_fd = nap->faulted_thread_fd_read;
    if (unconsumed_bytes_ < kBufSize) {
      FD_SET(handle_, &fds);
      max_fd = std::max(max_fd, handle_);
    }

    int ret;
    // We don't need sleep-polling on Linux now,
    // so we set either zero or infinite timeout.
    if (wait) {
      ret = select(max_fd + 1, &fds, NULL, NULL, NULL);
    } else {
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      ret = select(max_fd + 1, &fds, NULL, NULL, &timeout);
    }
    if (ret < 0) {
      NaClLog(LOG_FATAL,
              "TransportIPC::WaitForDebugStubEvent: Failed to wait for "
              "debug stub event.\n");
    }

    if (ret > 0) {
      if (FD_ISSET(nap->faulted_thread_fd_read, &fds)) {
        char buf[16];
        if (read(nap->faulted_thread_fd_read, &buf, sizeof(buf)) < 0) {
          NaClLog(LOG_FATAL,
                  "TransportIPC::WaitForDebugStubEvent: Failed to read from "
                  "debug stub event pipe fd.\n");
        }
      }
      if (FD_ISSET(handle_, &fds))
        FillBufferIfEmpty();
    }
  }

  virtual void Disconnect() {
    // If we are being marked as disconnected then we should also
    // receive the disconnect marker so the next connection is in
    // a proper state.
    if (IsConnected()) {
      do {
        unconsumed_bytes_ = 0;
        // FillBufferIfEmpty() returns false on disconnect or a
        // Fatal error, and in both cases we want to exit the loop.
      } while (FillBufferIfEmpty());
    }

    // Throw away unused data.
    unconsumed_bytes_ = 0;
  }

  // Block until we have new data on the pipe, new data means the
  // server socket across the pipe got a new connection.
  virtual bool AcceptConnection() {
    CHECK(!IsConnected());
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(handle_, &fds);

    // Check if this file handle can select on read
    int cnt = select(handle_ + 1, &fds, 0, 0, NULL);

    // If we are ready, or if there is an error.  We return true
    // on error, to let the next IO request fail.
    if (cnt != 0) {
      // This marks ourself as connected.
      bytes_to_read_ = 0;
      return true;
    }

    return false;
  }

 private:
  // Returns whether we are in a connected state. This refers to the
  // connection of the server across the pipe, if the pipe itself is
  // ever disconnected we will probably be crashing or exitting soon.
  bool IsConnected() {
    return bytes_to_read_ != kServerSocketDisconnect;
  }

  // Copy buffered data to *dst up to len bytes and update dst and len.
  void CopyFromBuffer(char **dst, int32_t *len) {
    int32_t copy_bytes = std::min(*len, unconsumed_bytes_);
    memcpy(*dst, buf_.get(), copy_bytes);
    // Keep data aligned with start of buffer.
    memmove(buf_.get(), buf_.get() + copy_bytes,
            unconsumed_bytes_ - copy_bytes);
    unconsumed_bytes_ -= copy_bytes;
    *len -= copy_bytes;
    *dst += copy_bytes;
  }

  // If the buffer is empty it tries to read data and returns true if
  // successful, otherwise if the buffer has data it returns true immediately.
  // Returns false if we got the disconnect marker from the other end of the
  // pipe meaning there was a server disconnect on the other side and we need
  // to wait for a new connection. Also return false on a fatal error.
  bool FillBufferIfEmpty() {
    if (unconsumed_bytes_ > 0)
      return true;

    if (bytes_to_read_ == 0) {
      // If this read fails it means something went wrong on other side.
      if (!ReadNBytes(reinterpret_cast<char *>(&bytes_to_read_),
                      sizeof(bytes_to_read_))) {
        NaClLog(LOG_FATAL,
                "TransportIPC::FillBufferIfEmpty: "
                "Pipe closed from other process.\n");
        // We want to mark ourselves as disconnected on an error.
        bytes_to_read_ = kServerSocketDisconnect;
        return false;
      }

      // If we got the disconnect flag mark it as such.
      if (bytes_to_read_ == kServerSocketDisconnect)
        return false;
    }

    int result = ::read(handle_, buf_.get() + unconsumed_bytes_,
                        std::min(bytes_to_read_, kBufSize));
    if (result > 0) {
      unconsumed_bytes_ += result;
      bytes_to_read_ -= result;
    } else if (result == 0 || errno != EINTR) {
      NaClLog(LOG_FATAL,
              "TransportIPC::FillBufferIfEmpty: "
              "Pipe closed from other process.\n");
      // We want to mark ourselves as disconnected on an error.
      bytes_to_read_ = kServerSocketDisconnect;
      return false;
    }

    return true;
  }

  // Block until you read len bytes.
  // Return false on EOF or error, but retries EINTR.
  bool ReadNBytes(char *buf, uint32_t len) {
    uint32_t bytes_read = 0;
    while (len > 0) {
      int result = ::read(handle_, buf + bytes_read, len);
      if (result > 0) {
        bytes_read += result;
        len -= result;
      } else if (result == 0 || errno != EINTR) {
        return false;
      }
    }
    return true;
  }

  static const int kServerSocketDisconnect = -1;
  static const int kBufSize = 4096;
  nacl::scoped_array<char> buf_;

  // Number of bytes stored in internal buffer.
  int32_t unconsumed_bytes_;

  // Number of bytes left in packet encoding from browser.
  int32_t bytes_to_read_;
  NaClHandle handle_;
};

// The constant is passed by reference in some cases. So
// under some optmizations or lack thereof it needs space.
const int TransportIPC::kBufSize;

#endif

ITransport *CreateTransportIPC(NaClHandle fd) {
#if NACL_LINUX || NACL_OSX
  return new TransportIPC(fd);
#else
  // TODO(leslieb): implement for windows.
  return NULL;
#endif
}

}  // namespace port
