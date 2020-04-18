// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_SIGNALING_FAKE_SIGNAL_STRATEGY_H_
#define REMOTING_SIGNALING_FAKE_SIGNAL_STRATEGY_H_

#include <list>
#include <queue>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "remoting/signaling/iq_sender.h"
#include "remoting/signaling/signal_strategy.h"
#include "remoting/signaling/signaling_address.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace remoting {

class FakeSignalStrategy : public SignalStrategy {
 public:
  // Calls ConenctTo() to connect |peer1| and |peer2|. Both |peer1| and |peer2|
  // must belong to the current thread.
  static void Connect(FakeSignalStrategy* peer1, FakeSignalStrategy* peer2);

  FakeSignalStrategy(const SignalingAddress& address);
  ~FakeSignalStrategy() override;

  const std::vector<std::unique_ptr<buzz::XmlElement>>& received_messages() {
    return received_messages_;
  }

  void set_send_delay(base::TimeDelta delay) {
    send_delay_ = delay;
  }

  void SetState(State state) const;

  // Connects current FakeSignalStrategy to receive messages from |peer|.
  void ConnectTo(FakeSignalStrategy* peer);

  void SetLocalAddress(const SignalingAddress& address);

  // Simulate IQ messages re-ordering by swapping the delivery order of
  // next pair of messages.
  void SimulateMessageReordering();

  // SignalStrategy interface.
  void Connect() override;
  void Disconnect() override;
  State GetState() const override;
  Error GetError() const override;
  const SignalingAddress& GetLocalAddress() const override;
  void AddListener(Listener* listener) override;
  void RemoveListener(Listener* listener) override;
  bool SendStanza(std::unique_ptr<buzz::XmlElement> stanza) override;
  std::string GetNextId() override;

 private:
  typedef base::Callback<void(std::unique_ptr<buzz::XmlElement> message)>
      PeerCallback;

  static void DeliverMessageOnThread(
      scoped_refptr<base::SingleThreadTaskRunner> thread,
      base::WeakPtr<FakeSignalStrategy> target,
      std::unique_ptr<buzz::XmlElement> stanza);

  // Called by the |peer_|. Takes ownership of |stanza|.
  void OnIncomingMessage(std::unique_ptr<buzz::XmlElement> stanza);
  void NotifyListeners(std::unique_ptr<buzz::XmlElement> stanza);
  void SetPeerCallback(const PeerCallback& peer_callback);

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_;

  State state_ = CONNECTED;

  SignalingAddress address_;
  PeerCallback peer_callback_;
  base::ObserverList<Listener, true> listeners_;

  int last_id_;

  base::TimeDelta send_delay_;

  bool simulate_reorder_ = false;
  std::unique_ptr<buzz::XmlElement> pending_stanza_;

  // All received messages, includes thouse still in |pending_messages_|.
  std::vector<std::unique_ptr<buzz::XmlElement>> received_messages_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<FakeSignalStrategy> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FakeSignalStrategy);
};

}  // namespace remoting

#endif  // REMOTING_SIGNALING_FAKE_SIGNAL_STRATEGY_H_
