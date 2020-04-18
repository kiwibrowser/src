// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/zygote/common/send_zygote_child_ping_linux.h"

#include <vector>

#include "base/posix/unix_domain_socket.h"
#include "services/service_manager/zygote/common/zygote_commands_linux.h"

namespace service_manager {

bool SendZygoteChildPing(int fd) {
  return base::UnixDomainSocket::SendMsg(fd, kZygoteChildPingMessage,
                                         sizeof(kZygoteChildPingMessage),
                                         std::vector<int>());
}

}  // namespace service_manager
