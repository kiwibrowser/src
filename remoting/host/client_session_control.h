// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_CLIENT_SESSION_CONTROL_H_
#define REMOTING_HOST_CLIENT_SESSION_CONTROL_H_

#include "remoting/protocol/errors.h"

namespace webrtc {
class DesktopVector;
}  // namespace webrtc

namespace remoting {

// Allows the desktop environment to disconnect the client session and
// to control the remote input handling (i.e. disable, enable, and pause
// temporarily if the local mouse movements are detected).
class ClientSessionControl {
 public:
  virtual ~ClientSessionControl() {}

  // Returns the authenticated JID of the client session.
  virtual const std::string& client_jid() const = 0;

  // Disconnects the client session, tears down transport resources and stops
  // scheduler components.
  virtual void DisconnectSession(protocol::ErrorCode error) = 0;

  // Called when local mouse movement is detected.
  virtual void OnLocalMouseMoved(const webrtc::DesktopVector& position) = 0;

  // Disables or enables the remote input in the client session.
  virtual void SetDisableInputs(bool disable_inputs) = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_CLIENT_SESSION_CONTROL_H_
