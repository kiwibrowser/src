// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_HOST_CHANGE_NOTIFICATION_LISTENER_H
#define REMOTING_HOST_HOST_CHANGE_NOTIFICATION_LISTENER_H

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "remoting/signaling/signal_strategy.h"

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

// HostChangeNotificationListener listens for messages from the remoting bot
// indicating that its host entry has been changed in the directory.
// If a message is received indicating that the host was deleted, it uses the
// OnHostDeleted callback to shut down the host.
class HostChangeNotificationListener : public SignalStrategy::Listener {
 public:
  class Listener {
   protected:
    virtual ~Listener() {}
    // Invoked when a notification that the host was deleted is received.
   public:
    virtual void OnHostDeleted() = 0;
  };

  // Both listener and signal_strategy are expected to outlive this object.
  HostChangeNotificationListener(Listener* listener,
                                 const std::string& host_id,
                                 SignalStrategy* signal_strategy,
                                 const std::string& directory_bot_jid);
  ~HostChangeNotificationListener() override;

  // SignalStrategy::Listener interface.
  void OnSignalStrategyStateChange(SignalStrategy::State state) override;
  bool OnSignalStrategyIncomingStanza(const buzz::XmlElement* stanza) override;

 private:
  void OnHostDeleted();

  Listener* listener_;
  std::string host_id_;
  SignalStrategy* signal_strategy_;
  std::string directory_bot_jid_;
  base::WeakPtrFactory<HostChangeNotificationListener> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(HostChangeNotificationListener);
};

}  // namespace remoting

#endif  // REMOTING_HOST_HOST_CHANGE_NOTIFICATION_LISTENER_H
