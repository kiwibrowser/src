// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_INVITATION_DISPATCHER_H_
#define MOJO_EDK_SYSTEM_INVITATION_DISPATCHER_H_

#include <stdint.h>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/dispatcher.h"
#include "mojo/edk/system/ports/port_ref.h"
#include "mojo/edk/system/system_impl_export.h"

namespace mojo {
namespace edk {

class MOJO_SYSTEM_IMPL_EXPORT InvitationDispatcher : public Dispatcher {
 public:
  InvitationDispatcher();

  // Dispatcher:
  Type GetType() const override;
  MojoResult Close() override;
  MojoResult AttachMessagePipe(base::StringPiece name,
                               ports::PortRef remote_peer_port) override;
  MojoResult ExtractMessagePipe(base::StringPiece name,
                                MojoHandle* message_pipe_handle) override;

  using PortMapping = base::flat_map<std::string, ports::PortRef>;
  PortMapping TakeAttachedPorts();

 private:
  ~InvitationDispatcher() override;

  base::Lock lock_;
  bool is_closed_ = false;
  PortMapping attached_ports_;

  DISALLOW_COPY_AND_ASSIGN(InvitationDispatcher);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_INVITATION_DISPATCHER_H
