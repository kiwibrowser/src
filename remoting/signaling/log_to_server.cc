// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/log_to_server.h"

#include <utility>

#include "remoting/base/constants.h"
#include "remoting/signaling/iq_sender.h"
#include "remoting/signaling/signal_strategy.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"
#include "third_party/libjingle_xmpp/xmpp/constants.h"

using buzz::QName;
using buzz::XmlElement;

namespace remoting {

LogToServer::LogToServer(ServerLogEntry::Mode mode,
                         SignalStrategy* signal_strategy,
                         const std::string& directory_bot_jid)
    : mode_(mode),
      signal_strategy_(signal_strategy),
      directory_bot_jid_(directory_bot_jid) {
  signal_strategy_->AddListener(this);
}

LogToServer::~LogToServer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  signal_strategy_->RemoveListener(this);
}

void LogToServer::OnSignalStrategyStateChange(SignalStrategy::State state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (state == SignalStrategy::CONNECTED) {
    iq_sender_.reset(new IqSender(signal_strategy_));
    SendPendingEntries();
  } else if (state == SignalStrategy::DISCONNECTED) {
    iq_sender_.reset();
  }
}

bool LogToServer::OnSignalStrategyIncomingStanza(
    const buzz::XmlElement* stanza) {
  return false;
}

void LogToServer::Log(const ServerLogEntry& entry) {
  pending_entries_.push_back(entry);
  SendPendingEntries();
}

void LogToServer::SendPendingEntries() {
  if (iq_sender_ == nullptr) {
    return;
  }
  if (pending_entries_.empty()) {
    return;
  }
  // Make one stanza containing all the pending entries.
  std::unique_ptr<XmlElement> stanza(ServerLogEntry::MakeStanza());
  while (!pending_entries_.empty()) {
    ServerLogEntry& entry = pending_entries_.front();
    stanza->AddElement(entry.ToStanza().release());
    pending_entries_.pop_front();
  }
  // Send the stanza to the server and ignore the response.
  iq_sender_->SendIq(buzz::STR_SET, directory_bot_jid_, std::move(stanza),
                     IqSender::ReplyCallback());
}

}  // namespace remoting
