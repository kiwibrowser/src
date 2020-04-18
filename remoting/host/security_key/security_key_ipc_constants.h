// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_SECURITY_KEY_SECURITY_KEY_IPC_CONSTANTS_H_
#define REMOTING_HOST_SECURITY_KEY_SECURITY_KEY_IPC_CONSTANTS_H_

#include <string>

#include "mojo/edk/embedder/named_platform_handle.h"

namespace remoting {

// Used to indicate an error during a security key forwarding session.
extern const char kSecurityKeyConnectionError[];

// Returns the name of the well-known IPC server channel used to initiate a
// security key forwarding session.
const mojo::edk::NamedPlatformHandle& GetSecurityKeyIpcChannel();

// Sets the name of the well-known IPC server channel for testing purposes.
void SetSecurityKeyIpcChannelForTest(
    const mojo::edk::NamedPlatformHandle& channel_handle);

// Returns a path appropriate for placing a channel name.  Without this path
// prefix, we may not have permission on linux to bind(2) a socket to a name in
// the current directory.
std::string GetChannelNamePathPrefixForTest();

}  // namespace remoting

#endif  // REMOTING_HOST_SECURITY_KEY_SECURITY_KEY_IPC_CONSTANTS_H_
