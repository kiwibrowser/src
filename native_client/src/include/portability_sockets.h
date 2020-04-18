/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_SOCKETS_H_
#define NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_SOCKETS_H_ 1

#include "native_client/src/include/build_config.h"

#if NACL_WINDOWS
# include <winsock2.h>
# include <windows.h>

typedef SOCKET NaClSocketHandle;

# define NaClCloseSocket closesocket
# define NACL_INVALID_SOCKET INVALID_SOCKET
# define NaClSocketGetLastError() WSAGetLastError()
#else

# include <arpa/inet.h>
# include <netdb.h>
# include <netinet/tcp.h>
# include <sys/select.h>
# include <sys/socket.h>

typedef int NaClSocketHandle;

# define NaClCloseSocket close
# define NACL_INVALID_SOCKET (-1)
# define NaClSocketGetLastError() errno

#endif

#endif  // NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_SOCKETS_H_
