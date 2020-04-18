/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
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
#include "native_client/src/include/portability_sockets.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/debug_stub/transport.h"
#include "native_client/src/trusted/debug_stub/util.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

using gdb_rsp::stringvec;
using gdb_rsp::StringSplit;

#if NACL_WINDOWS
typedef int socklen_t;
#endif

namespace port {

class Transport : public ITransport {
 public:
  Transport()
    : buf_(new char[kBufSize]),
      pos_(0),
      size_(0) {
    handle_bind_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    handle_accept_ = NACL_INVALID_SOCKET;
  }

  explicit Transport(NaClSocketHandle s)
    : buf_(new char[kBufSize]),
      pos_(0),
      size_(0),
      handle_bind_(s),
      handle_accept_(NACL_INVALID_SOCKET) { }

  ~Transport() {
    if (handle_accept_ != NACL_INVALID_SOCKET) NaClCloseSocket(handle_accept_);
#if NACL_WINDOWS
    if (!WSACloseEvent(socket_event_)) {
      NaClLog(LOG_FATAL,
              "Transport::~Transport: Failed to close socket event\n");
    }
#endif
  }

#if NACL_WINDOWS
  void CreateSocketEvent() {
    socket_event_ = WSACreateEvent();
    if (socket_event_ == WSA_INVALID_EVENT) {
      NaClLog(LOG_FATAL,
              "Transport::CreateSocketEvent: Failed to create socket event\n");
    }
    // Listen for close events in order to handle them correctly.
    // Additionally listen for read readiness as WSAEventSelect sets the socket
    // to non-blocking mode.
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms738547(v=vs.85).aspx
    if (WSAEventSelect(
          handle_accept_, socket_event_, FD_CLOSE | FD_READ) == SOCKET_ERROR) {
      NaClLog(LOG_FATAL,
              "Transport::CreateSocketEvent: Failed to bind event to socket\n");
    }
  }
#endif

  // Read from this transport, return true on success.
  virtual bool Read(void *ptr, int32_t len);

  // Write to this transport, return true on success.
  virtual bool Write(const void *ptr, int32_t len);

  // Return true if there is data to read.
  virtual bool IsDataAvailable() {
    if (pos_ < size_) {
      return true;
    }
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(handle_accept_, &fds);

    // We want a "non-blocking" check
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Check if this file handle can select on read
    int cnt = select(static_cast<int>(handle_accept_) + 1,
                                      &fds, 0, 0, &timeout);

    // If we are ready, or if there is an error.  We return true
    // on error, to let the next IO request fail.
    if (cnt != 0) return true;

    return false;
  }

  virtual void WaitForDebugStubEvent(struct NaClApp *nap,
                                     bool ignore_input_from_gdb);

// On windows, the header that defines this has other definition
// colitions, so we define it outselves just in case
#ifndef SD_BOTH
#define SD_BOTH 2
#endif

  virtual void Disconnect() {
    // Shutdown the conneciton in both diections.  This should
    // always succeed, and nothing we can do if this fails.
    (void) ::shutdown(handle_accept_, SD_BOTH);

    if (handle_accept_ != NACL_INVALID_SOCKET) NaClCloseSocket(handle_accept_);
#if NACL_WINDOWS
    if (!WSACloseEvent(socket_event_)) {
      NaClLog(LOG_FATAL,
              "Transport::~Transport: Failed to close socket event\n");
    }
    socket_event_ = WSA_INVALID_EVENT;
#endif
    handle_accept_ = NACL_INVALID_SOCKET;
  }

  virtual bool AcceptConnection() {
    CHECK(handle_accept_ == NACL_INVALID_SOCKET);
    handle_accept_ = ::accept(handle_bind_, NULL, 0);
    if (handle_accept_ != NACL_INVALID_SOCKET) {
      // Do not delay sending small packets.  This significantly speeds up
      // remote debugging.  Debug stub uses buffering to send outgoing packets
      // so they are not split into more TCP packets than necessary.
      int nodelay = 1;
      if (setsockopt(handle_accept_, IPPROTO_TCP, TCP_NODELAY,
                     reinterpret_cast<char *>(&nodelay),
                     sizeof(nodelay))) {
        NaClLog(LOG_WARNING, "Failed to set TCP_NODELAY option.\n");
      }
      #if NACL_WINDOWS
        CreateSocketEvent();
      #endif
      return true;
    }
    return false;
  }

 protected:
  // Copy buffered data to *dst up to len bytes and update dst and len.
  void CopyFromBuffer(char **dst, int32_t *len);

  // Read available data from the socket. Return false on EOF or error.
  bool ReadSomeData();

  static const int kBufSize = 4096;
  nacl::scoped_array<char> buf_;
  int32_t pos_;
  int32_t size_;
  NaClSocketHandle handle_bind_;
  NaClSocketHandle handle_accept_;
#if NACL_WINDOWS
  HANDLE socket_event_;
#endif
};

void Transport::CopyFromBuffer(char **dst, int32_t *len) {
  int32_t copy_bytes = std::min(*len, size_ - pos_);
  memcpy(*dst, buf_.get() + pos_, copy_bytes);
  pos_ += copy_bytes;
  *len -= copy_bytes;
  *dst += copy_bytes;
}

bool Transport::ReadSomeData() {
  while (true) {
    int result = ::recv(handle_accept_,
                        buf_.get() + size_,
                        kBufSize - size_, 0);
    if (result > 0) {
      size_ += result;
      return true;
    }
    if (result == 0)
      return false;
#if NACL_WINDOWS
    // WSAEventSelect sets socket to non-blocking mode. This is essential
    // for socket event notification to work, there is no workaround.
    // See remarks section at the page
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms741576(v=vs.85).aspx
    if (NaClSocketGetLastError() == WSAEWOULDBLOCK) {
      if (WaitForSingleObject(socket_event_, INFINITE) == WAIT_FAILED) {
        NaClLog(LOG_FATAL,
                "Transport::ReadSomeData: Failed to wait on socket event\n");
      }
      if (!ResetEvent(socket_event_)) {
        NaClLog(LOG_FATAL,
                "Transport::ReadSomeData: Failed to reset socket event\n");
      }
      continue;
    }
#endif
    if (NaClSocketGetLastError() != EINTR)
      return false;
  }
}

bool Transport::Read(void *ptr, int32_t len) {
  char *dst = static_cast<char *>(ptr);
  if (pos_ < size_) {
    CopyFromBuffer(&dst, &len);
  }
  while (len > 0) {
    pos_ = 0;
    size_ = 0;
    if (!ReadSomeData()) {
      return false;
    }
    CopyFromBuffer(&dst, &len);
  }
  return true;
}

bool Transport::Write(const void *ptr, int32_t len) {
  const char *src = static_cast<const char *>(ptr);
  while (len > 0) {
    int result = ::send(handle_accept_, src, len, 0);
    if (result > 0) {
      src += result;
      len -= result;
      continue;
    }
    if (result == 0) {
      return false;
    }
    if (NaClSocketGetLastError() != EINTR) {
      return false;
    }
  }
  return true;
}

void Transport::WaitForDebugStubEvent(struct NaClApp *nap,
                                      bool ignore_input_from_gdb) {
  bool wait = true;
  // If we are told to ignore messages from gdb, we will exit from this
  // function only if new data is sent by gdb.
  if ((pos_ < size_ && !ignore_input_from_gdb) ||
      nap->faulted_thread_count > 0) {
    // Clear faulted thread events to save debug stub loop iterations.
    wait = false;
  }
#if NACL_WINDOWS
  HANDLE handles[2];
  handles[0] = nap->faulted_thread_event;
  handles[1] = socket_event_;
  int count = size_ < kBufSize ? 2 : 1;
  int result = WaitForMultipleObjects(count, handles, FALSE,
                                      wait ? INFINITE : 0);
  if (result == WAIT_OBJECT_0 + 1) {
    if (!ResetEvent(socket_event_)) {
      NaClLog(LOG_FATAL,
              "Transport::WaitForDebugStubEvent: "
              "Failed to reset socket event\n");
    }
    return;
  }
  if (result == WAIT_TIMEOUT || result == WAIT_OBJECT_0)
    return;
  NaClLog(LOG_FATAL,
          "Transport::WaitForDebugStubEvent: Wait for events failed\n");
#else
  fd_set fds;

  FD_ZERO(&fds);
  FD_SET(nap->faulted_thread_fd_read, &fds);
  int max_fd = nap->faulted_thread_fd_read;
  if (size_ < kBufSize) {
    FD_SET(handle_accept_, &fds);
    max_fd = std::max(max_fd, handle_accept_);
  }

  int ret;
  // We don't need sleep-polling on Linux now, so we set either zero or infinite
  // timeout.
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
            "Transport::WaitForDebugStubEvent: Failed to wait for "
            "debug stub event\n");
  }

  if (ret > 0) {
    if (FD_ISSET(nap->faulted_thread_fd_read, &fds)) {
      char buf[16];
      if (read(nap->faulted_thread_fd_read, &buf, sizeof(buf)) < 0) {
        NaClLog(LOG_FATAL,
                "Transport::WaitForDebugStubEvent: Failed to read from "
                "debug stub event pipe fd\n");
      }
    }
    if (FD_ISSET(handle_accept_, &fds))
      ReadSomeData();
  }
#endif
}

// Convert string in the form of [addr][:port] where addr is a
// IPv4 address or host name, and port is a 16b tcp/udp port.
// Both portions are optional, and only the portion of the address
// provided is updated.  Values are provided in network order.
static bool StringToIPv4(const std::string &instr, uint32_t *addr,
                         uint16_t *port) {
  // Make a copy so the are unchanged unless we succeed
  uint32_t outaddr = *addr;
  uint16_t outport = *port;

  // Substrings of the full ADDR:PORT
  std::string addrstr;
  std::string portstr;

  // We should either have one or two tokens in the form of:
  //  IP - IP, NUL
  //  IP: -  IP, NUL
  //  :PORT - NUL, PORT
  //  IP:PORT - IP, PORT

  // Search for the port marker
  size_t portoff = instr.find(':');

  // If we found a ":" before the end, get both substrings
  if ((portoff != std::string::npos) && (portoff + 1 < instr.size())) {
    addrstr = instr.substr(0, portoff);
    portstr = instr.substr(portoff + 1, std::string::npos);
  } else {
    // otherwise the entire string is the addr portion.
    addrstr = instr;
    portstr = "";
  }

  // If the address portion was provided, update it
  if (addrstr.size()) {
    // Special case 0.0.0.0 which means any IPv4 interface
    if (addrstr == "0.0.0.0") {
      outaddr = 0;
    } else {
      struct hostent *host = gethostbyname(addrstr.data());

      // Check that we found an IPv4 host
      if ((NULL == host) || (AF_INET != host->h_addrtype)) return false;

      // Make sure the IP list isn't empty.
      if (0 == host->h_addr_list[0]) return false;

      // Use the first address in the array of address pointers.
      uint32_t **addrarray = reinterpret_cast<uint32_t**>(host->h_addr_list);
      outaddr = *addrarray[0];
    }
  }

  // if the port portion was provided, then update it
  if (portstr.size()) {
    int val = atoi(portstr.data());
    if ((val < 0) || (val > 65535)) return false;
    outport = ntohs(static_cast<uint16_t>(val));
  }

  // We haven't failed, so set the values
  *addr = outaddr;
  *port = outport;
  return true;
}

static bool BuildSockAddr(const char *addr, struct sockaddr_in *sockaddr) {
  std::string addrstr = addr;
  uint32_t *pip = reinterpret_cast<uint32_t*>(&sockaddr->sin_addr.s_addr);
  uint16_t *pport = reinterpret_cast<uint16_t*>(&sockaddr->sin_port);

  sockaddr->sin_family = AF_INET;
  return StringToIPv4(addrstr, pip, pport);
}

SocketBinding::SocketBinding(NaClSocketHandle socket_handle)
    : socket_handle_(socket_handle) {
}

SocketBinding *SocketBinding::Bind(const char *addr) {
  NaClSocketHandle socket_handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_handle == NACL_INVALID_SOCKET) {
    NaClLog(LOG_ERROR, "Failed to create socket.\n");
    return NULL;
  }
  struct sockaddr_in saddr;
  // Clearing sockaddr_in first appears to be necessary on Mac OS X.
  memset(&saddr, 0, sizeof(saddr));
  socklen_t addrlen = static_cast<socklen_t>(sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(0x7F000001);
  saddr.sin_port = htons(4014);

  // Override portions address that are provided
  if (addr) BuildSockAddr(addr, &saddr);

#if NACL_WINDOWS
  // On Windows, SO_REUSEADDR has a different meaning than on POSIX systems.
  // SO_REUSEADDR allows hijacking of an open socket by another process.
  // The SO_EXCLUSIVEADDRUSE flag prevents this behavior.
  // See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms740621(v=vs.85).aspx
  //
  // Additionally, unlike POSIX, TCP server sockets can be bound to
  // ports in the TIME_WAIT state, without setting SO_REUSEADDR.
  int exclusive_address = 1;
  if (setsockopt(socket_handle, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                 reinterpret_cast<char *>(&exclusive_address),
                 sizeof(exclusive_address))) {
    NaClLog(LOG_WARNING, "Failed to set SO_EXCLUSIVEADDRUSE option.\n");
  }
#else
  // On POSIX, this is necessary to ensure that the TCP port is released
  // promptly when sel_ldr exits.  Without this, the TCP port might
  // only be released after a timeout, and later processes can fail
  // to bind it.
  int reuse_address = 1;
  if (setsockopt(socket_handle, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<char *>(&reuse_address),
                 sizeof(reuse_address))) {
    NaClLog(LOG_WARNING, "Failed to set SO_REUSEADDR option.\n");
  }
#endif

  struct sockaddr *psaddr = reinterpret_cast<struct sockaddr *>(&saddr);
  if (bind(socket_handle, psaddr, addrlen)) {
    NaClLog(LOG_ERROR, "Failed to bind server.\n");
    return NULL;
  }

  if (listen(socket_handle, 1)) {
    NaClLog(LOG_ERROR, "Failed to listen.\n");
    return NULL;
  }
  return new SocketBinding(socket_handle);
}

ITransport *SocketBinding::CreateTransport() {
  return new Transport(socket_handle_);
}

uint16_t SocketBinding::GetBoundPort() {
  struct sockaddr_in saddr;
  struct sockaddr *psaddr = reinterpret_cast<struct sockaddr *>(&saddr);
  // Clearing sockaddr_in first appears to be necessary on Mac OS X.
  memset(&saddr, 0, sizeof(saddr));
  socklen_t addrlen = static_cast<socklen_t>(sizeof(saddr));
  if (::getsockname(socket_handle_, psaddr, &addrlen)) {
    NaClLog(LOG_ERROR, "Failed to retrieve bound address.\n");
    return 0;
  }
  return ntohs(saddr.sin_port);
}

}  // namespace port
