// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_SIGNALING_SIGNAL_STRATEGY_H_
#define REMOTING_SIGNALING_SIGNAL_STRATEGY_H_

#include <memory>
#include <string>

#include "base/macros.h"

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

class SignalingAddress;

class SignalStrategy {
 public:
  enum State {
    // Connection is being established.
    CONNECTING,

    // Signalling is connected.
    CONNECTED,

    // Connection is closed due to an error or because Disconnect()
    // was called.
    DISCONNECTED,
  };

  enum Error {
    OK,
    AUTHENTICATION_FAILED,
    NETWORK_ERROR,
    PROTOCOL_ERROR,
  };

  // Callback interface for signaling event. Event handlers are not
  // allowed to destroy SignalStrategy, but may add or remove other
  // listeners.
  class Listener {
   public:
    virtual ~Listener() {}

    // Called after state of the connection has changed. If the state
    // is DISCONNECTED, then GetError() can be used to get the reason
    // for the disconnection.
    virtual void OnSignalStrategyStateChange(State state) = 0;

    // Must return true if the stanza was handled, false
    // otherwise. The signal strategy must not be deleted from a
    // handler of this message.
    virtual bool OnSignalStrategyIncomingStanza(
        const buzz::XmlElement* stanza) = 0;
  };

  SignalStrategy() {}
  virtual ~SignalStrategy() {}

  // Starts connection attempt. If connection is currently active
  // disconnects it and opens a new connection (implicit disconnect
  // triggers CLOSED notification). Connection is finished
  // asynchronously.
  virtual void Connect() = 0;

  // Disconnects current connection if connected. Triggers CLOSED
  // notification.
  virtual void Disconnect() = 0;

  // Returns current state.
  virtual State GetState() const = 0;

  // Returns the last error. Set when state changes to DISCONNECT.
  virtual Error GetError() const = 0;

  // Local address. An empty value is returned when not connected.
  virtual const SignalingAddress& GetLocalAddress() const = 0;

  // Add a |listener| that can listen to all incoming
  // messages. Doesn't take ownership of the |listener|. All listeners
  // must be removed before this object is destroyed.
  virtual void AddListener(Listener* listener) = 0;

  // Remove a |listener| previously added with AddListener().
  virtual void RemoveListener(Listener* listener) = 0;

  // Sends a raw XMPP stanza. Returns false if the stanza couldn't be send.
  virtual bool SendStanza(std::unique_ptr<buzz::XmlElement> stanza) = 0;

  // Returns new ID that should be used for the next outgoing IQ
  // request.
  virtual std::string GetNextId() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(SignalStrategy);
};

}  // namespace remoting

#endif  // REMOTING_SIGNALING_SIGNAL_STRATEGY_H_
