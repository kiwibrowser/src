// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/win/sandbox_poc/pocdll/exports.h"
#include "sandbox/win/sandbox_poc/pocdll/utils.h"

// This file contains the tests used to verify the security of the network.

void POCDLL_API TestNetworkListen(HANDLE log) {
  HandleToFile handle2file;
  FILE *output = handle2file.Translate(log, "w");
#if DONT_WANT_INTERCEPTIONS_JUST_WANT_NETWORK
  // Initialize Winsock
  WSADATA wsa_data;
  int result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (result != NO_ERROR) {
    fprintf(output, "[ERROR] Cannot initialize winsock. Error%d\r\n", result);
    return;
  }

  // Create a SOCKET for listening for
  // incoming connection requests.
  SOCKET listen_socket;
  listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listen_socket == INVALID_SOCKET) {
    fprintf(output, "[ERROR] Failed to create socket. Error %ld\r\n",
           ::WSAGetLastError());
    ::WSACleanup();
    return;
  }

  // The sockaddr_in structure specifies the address family,
  // IP address, and port for the socket that is being bound.
  sockaddr_in service;
  service.sin_family = AF_INET;
  service.sin_addr.s_addr = inet_addr("127.0.0.1");
  service.sin_port = htons(88);

  if (bind(listen_socket, reinterpret_cast<SOCKADDR*>(&service),
           sizeof(service)) == SOCKET_ERROR) {
    fprintf(output, "[BLOCKED] Bind socket on port 88. Error %ld\r\n",
            ::WSAGetLastError());
    closesocket(listen_socket);
    ::WSACleanup();
    return;
  }

  // Listen for incoming connection requests
  // on the created socket
  if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
    fprintf(output, "[BLOCKED] Listen socket on port 88. Error %ld\r\n",
            ::WSAGetLastError());

  } else {
    fprintf(output, "[GRANTED] Listen socket on port 88.\r\n",
            ::WSAGetLastError());
  }

  ::WSACleanup();
  return;
#else  // DONT_WANT_INTERCEPTIONS_JUST_WANT_NETWORK
  // Just print out that this test is not running.
  fprintf(output, "[ERROR] No network tests.\r\n");
#endif  // DONT_WANT_INTERCEPTIONS_JUST_WANT_NETWORK
}
