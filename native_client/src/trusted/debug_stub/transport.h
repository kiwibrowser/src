/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// This module provides interfaces for an IO stream.  The stream is
// expected to throw a std::exception if the stream is terminated on
// either side.
#ifndef NATIVE_CLIENT_PORT_TRANSPORT_H_
#define NATIVE_CLIENT_PORT_TRANSPORT_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_sockets.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"

struct NaClApp;

namespace port {

class SocketBinding;

class ITransport {
 public:
  virtual ~ITransport() {}  // Allow to delete using base pointer

  // Read from this transport, return true on success.
  virtual bool Read(void *ptr, int32_t len) = 0;

  // Write to this transport, return true on success.
  virtual bool Write(const void *ptr, int32_t len)  = 0;

  // Return true if there is data to read.
  virtual bool IsDataAvailable() = 0;

  // Wait until there is input from GDB or some NaCl thread is faulted.
  virtual void WaitForDebugStubEvent(struct NaClApp *nap,
                                     bool ignore_gdb) = 0;

  // Disconnect the transport, R/W and Select will now throw an exception
  virtual void Disconnect() = 0;

  // Accept a connection on an already-bound TCP port.
  virtual bool AcceptConnection() = 0;
};

class SocketBinding {
 public:
  // Wrap existing socket handle.
  explicit SocketBinding(NaClSocketHandle socket_handle);
  // Bind to the specified TCP port.
  static SocketBinding *Bind(const char *addr);

  // Create a transport object from this socket binding
  ITransport *CreateTransport();

  // Get port the socket is bound to.
  uint16_t GetBoundPort();

 private:
  NaClSocketHandle socket_handle_;
};

// Creates a transport from a socket pair or named pipe.
ITransport *CreateTransportIPC(NaClHandle fd);

}  // namespace port

#endif  // NATIVE_CLIENT_PORT_TRANSPORT_H_

