// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_OUTGOING_BROKER_CLIENT_INVITATION_H_
#define MOJO_EDK_EMBEDDER_OUTGOING_BROKER_CLIENT_INVITATION_H_

#include <stdint.h>

#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/process/process_handle.h"
#include "mojo/edk/embedder/connection_params.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/system/system_impl_export.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {
namespace edk {

namespace ports {
class PortRef;
}

// Any Mojo embedder which is either the broker process itself or an existing
// broker client can use a OutgoingBrokerClientInvitation to invite a new
// process into broker's group of connected processes.
//
// In order to use OutgoingBrokerClientInvitation, you must have a valid process
// handle to the target process, as well as a valid ConnectionParams
// corresponding to some ConnectionParams in the target process.
//
// It is the embedder's responsibility get a corresponding ConnectionParams into
// the target process somehow (for example, by using file descriptor inheritance
// at process launch to give it the other end of a socket pair) and ensure that
// the target process constructs a corresponding IncomingBrokerClientInvitation
// object for that ConnectionParams.
//
// New message pipes may be attached to a OutgoingBrokerClientInvitation by
// calling AttachMessagePipe().
//
// The invitation is sent by calling Send(), and once the invitation is sent
// there is no further need to keep the OutgoingBrokerClientInvitation object
// alive.
class MOJO_SYSTEM_IMPL_EXPORT OutgoingBrokerClientInvitation {
 public:
  OutgoingBrokerClientInvitation();
  ~OutgoingBrokerClientInvitation();

  // Attaches a new message pipe to this invitation. The returned message pipe
  // handle can be used immediately in the calling process. The other end can be
  // obtained by the eventual receiver of this invitation, i.e., the target of
  // Send().
  //
  // NOTE: This must not be called after Send().
  ScopedMessagePipeHandle AttachMessagePipe(const std::string& name);

  // Sends the invitation to the target process. |target_process| must be a
  // valid handle and |params| must correspond to some other ConnectionParams
  // (e.g. the other half of a socket pair) in the target process.
  void Send(
      base::ProcessHandle target_process,
      ConnectionParams params,
      const ProcessErrorCallback& error_callback = ProcessErrorCallback());

  // Extracts an attached message pipe endpoint by name. For use only when this
  // invitation will NOT be sent to a remote process (i.e. Send() will never be
  // be called) after all, and the caller wishes to retrieve the message pipe
  // endpoint that would have been received.
  //
  // TODO(rockot): Remove this. It's only here to support content single-process
  // mode and the NaCl broker, both of which could be implemented without this
  // after some refactoring.
  ScopedMessagePipeHandle ExtractInProcessMessagePipe(const std::string& name);

 private:
  // List of named ports attached to this invitation. Each port is the peer of
  // some corresponding message pipe handle returned by AttachMessagePipe.
  std::vector<std::pair<std::string, ports::PortRef>> attached_ports_;

  // Indicates whether the invitation has been sent yet.
  bool sent_ = false;

  DISALLOW_COPY_AND_ASSIGN(OutgoingBrokerClientInvitation);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_OUTGOING_BROKER_CLIENT_INVITATION_H_
