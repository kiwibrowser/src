// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/host_change_notification_listener.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "remoting/base/constants.h"
#include "remoting/protocol/jingle_messages.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"
#include "third_party/libjingle_xmpp/xmpp/constants.h"

using buzz::QName;
using buzz::XmlElement;

namespace remoting {

HostChangeNotificationListener::HostChangeNotificationListener(
    Listener* listener,
    const std::string& host_id,
    SignalStrategy* signal_strategy,
    const std::string& directory_bot_jid)
    : listener_(listener),
      host_id_(host_id),
      signal_strategy_(signal_strategy),
      directory_bot_jid_(directory_bot_jid),
      weak_factory_(this) {
  DCHECK(signal_strategy_);

  signal_strategy_->AddListener(this);
}

HostChangeNotificationListener::~HostChangeNotificationListener() {
  signal_strategy_->RemoveListener(this);
}

void HostChangeNotificationListener::OnSignalStrategyStateChange(
    SignalStrategy::State state) {
}

bool HostChangeNotificationListener::OnSignalStrategyIncomingStanza(
    const buzz::XmlElement* stanza) {
  if (stanza->Name() != buzz::QN_IQ || stanza->Attr(buzz::QN_TYPE) != "set")
    return false;

  const XmlElement* host_changed_element =
      stanza->FirstNamed(QName(kChromotingXmlNamespace, "host-changed"));
  if (!host_changed_element)
    return false;

  const std::string& host_id =
      host_changed_element->Attr(QName(kChromotingXmlNamespace, "hostid"));
  const std::string& from = stanza->Attr(buzz::QN_FROM);

  std::string to_error;
  SignalingAddress to =
      SignalingAddress::Parse(stanza, SignalingAddress::TO, &to_error);

  if (host_id == host_id_ && from == directory_bot_jid_ &&
      to == signal_strategy_->GetLocalAddress()) {
    const std::string& operation =
        host_changed_element->Attr(QName(kChromotingXmlNamespace, "operation"));
    if (operation == "delete") {
      // OnHostDeleted() may want delete |signal_strategy_|, but SignalStrategy
      // objects cannot be deleted from a Listener callback, so OnHostDeleted()
      // has to be invoked later.
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::Bind(&HostChangeNotificationListener::OnHostDeleted,
              weak_factory_.GetWeakPtr()));
    }
  } else {
    LOG(ERROR) << "Invalid host-changed message received: " << stanza->Str();
  }
  return true;
}

void HostChangeNotificationListener::OnHostDeleted() {
  listener_->OnHostDeleted();
}

}  // namespace remoting
