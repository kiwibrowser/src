// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/fake_signal_strategy.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "remoting/signaling/jid_util.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"
#include "third_party/libjingle_xmpp/xmpp/constants.h"

namespace remoting {

// static
void FakeSignalStrategy::Connect(FakeSignalStrategy* peer1,
                                 FakeSignalStrategy* peer2) {
  DCHECK(peer1->main_thread_->BelongsToCurrentThread());
  DCHECK(peer2->main_thread_->BelongsToCurrentThread());
  peer1->ConnectTo(peer2);
  peer2->ConnectTo(peer1);
}

FakeSignalStrategy::FakeSignalStrategy(const SignalingAddress& address)
    : main_thread_(base::ThreadTaskRunnerHandle::Get()),
      address_(address),
      last_id_(0),
      weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

FakeSignalStrategy::~FakeSignalStrategy() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void FakeSignalStrategy::ConnectTo(FakeSignalStrategy* peer) {
  PeerCallback peer_callback =
      base::Bind(&FakeSignalStrategy::DeliverMessageOnThread,
                 main_thread_,
                 weak_factory_.GetWeakPtr());
  if (peer->main_thread_->BelongsToCurrentThread()) {
    peer->SetPeerCallback(peer_callback);
  } else {
    peer->main_thread_->PostTask(
        FROM_HERE, base::Bind(&FakeSignalStrategy::SetPeerCallback,
                              base::Unretained(peer), peer_callback));
  }
}

void FakeSignalStrategy::SetLocalAddress(const SignalingAddress& address) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  address_ = address;
}

void FakeSignalStrategy::SimulateMessageReordering() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  simulate_reorder_ = true;
}

void FakeSignalStrategy::Connect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  state_ = CONNECTED;
  for (auto& observer : listeners_)
    observer.OnSignalStrategyStateChange(CONNECTED);
}

void FakeSignalStrategy::Disconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  state_ = DISCONNECTED;
  for (auto& observer : listeners_)
    observer.OnSignalStrategyStateChange(DISCONNECTED);
}

SignalStrategy::State FakeSignalStrategy::GetState() const {
  return state_;
}

SignalStrategy::Error FakeSignalStrategy::GetError() const {
  return OK;
}

const SignalingAddress& FakeSignalStrategy::GetLocalAddress() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return address_;
}

void FakeSignalStrategy::AddListener(Listener* listener) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  listeners_.AddObserver(listener);
}

void FakeSignalStrategy::RemoveListener(Listener* listener) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  listeners_.RemoveObserver(listener);
}

bool FakeSignalStrategy::SendStanza(std::unique_ptr<buzz::XmlElement> stanza) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  address_.SetInMessage(stanza.get(), SignalingAddress::FROM);

  if (peer_callback_.is_null())
    return false;

  if (send_delay_.is_zero()) {
    peer_callback_.Run(std::move(stanza));
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::Bind(peer_callback_, base::Passed(&stanza)),
        send_delay_);
  }
  return true;
}

std::string FakeSignalStrategy::GetNextId() {
  ++last_id_;
  return base::IntToString(last_id_);
}

// static
void FakeSignalStrategy::DeliverMessageOnThread(
    scoped_refptr<base::SingleThreadTaskRunner> thread,
    base::WeakPtr<FakeSignalStrategy> target,
    std::unique_ptr<buzz::XmlElement> stanza) {
  thread->PostTask(FROM_HERE,
                   base::Bind(&FakeSignalStrategy::OnIncomingMessage,
                              target, base::Passed(&stanza)));
}

void FakeSignalStrategy::OnIncomingMessage(
    std::unique_ptr<buzz::XmlElement> stanza) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!simulate_reorder_) {
    NotifyListeners(std::move(stanza));
    return;
  }

  // Simulate IQ messages re-ordering by swapping the delivery order of
  // next pair of messages.
  if (pending_stanza_) {
    NotifyListeners(std::move(stanza));
    NotifyListeners(std::move(pending_stanza_));
    pending_stanza_.reset();
  } else {
    pending_stanza_ = std::move(stanza);
  }
}

void FakeSignalStrategy::NotifyListeners(
    std::unique_ptr<buzz::XmlElement> stanza) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  buzz::XmlElement* stanza_ptr = stanza.get();
  received_messages_.push_back(std::move(stanza));

  std::string to_error;
  SignalingAddress to =
      SignalingAddress::Parse(stanza_ptr, SignalingAddress::TO, &to_error);
  if (to != address_) {
    LOG(WARNING) << "Dropping stanza that is addressed to " << to.id()
                 << ". Local address: " << address_.id()
                 << ". Message content: " << stanza_ptr->Str();
    return;
  }

  for (auto& listener : listeners_) {
    if (listener.OnSignalStrategyIncomingStanza(stanza_ptr))
      break;
  }
}

void FakeSignalStrategy::SetPeerCallback(const PeerCallback& peer_callback) {
  peer_callback_ = peer_callback;
}

}  // namespace remoting
