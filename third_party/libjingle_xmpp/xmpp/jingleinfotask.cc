/*
 *  Copyright 2010 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "third_party/libjingle_xmpp/xmpp/jingleinfotask.h"

#include "third_party/libjingle_xmpp/xmpp/constants.h"
#include "third_party/libjingle_xmpp/xmpp/xmppclient.h"
#include "third_party/libjingle_xmpp/xmpp/xmpptask.h"
#include "third_party/webrtc/rtc_base/socketaddress.h"

namespace buzz {

class JingleInfoTask::JingleInfoGetTask : public XmppTask {
 public:
  explicit JingleInfoGetTask(XmppTaskParentInterface* parent)
      : XmppTask(parent, XmppEngine::HL_SINGLE), done_(false) {}

  virtual int ProcessStart() {
    rtc::scoped_ptr<XmlElement> get(MakeIq(STR_GET, Jid(), task_id()));
    get->AddElement(new XmlElement(QN_JINGLE_INFO_QUERY, true));
    if (SendStanza(get.get()) != XMPP_RETURN_OK) {
      return STATE_ERROR;
    }
    return STATE_RESPONSE;
  }
  virtual int ProcessResponse() {
    if (done_)
      return STATE_DONE;
    return STATE_BLOCKED;
  }

 protected:
  virtual bool HandleStanza(const XmlElement* stanza) {
    if (!MatchResponseIq(stanza, Jid(), task_id()))
      return false;

    if (stanza->Attr(QN_TYPE) != STR_RESULT)
      return false;

    // Queue the stanza with the parent so these don't get handled out of order
    JingleInfoTask* parent = static_cast<JingleInfoTask*>(GetParent());
    parent->QueueStanza(stanza);

    // Wake ourselves so we can go into the done state
    done_ = true;
    Wake();
    return true;
  }

  bool done_;
};

void JingleInfoTask::RefreshJingleInfoNow() {
  JingleInfoGetTask* get_task = new JingleInfoGetTask(this);
  get_task->Start();
}

bool JingleInfoTask::HandleStanza(const XmlElement* stanza) {
  if (!MatchRequestIq(stanza, "set", QN_JINGLE_INFO_QUERY))
    return false;

  // only respect relay push from the server
  Jid from(stanza->Attr(QN_FROM));
  if (!from.IsEmpty() && !from.BareEquals(GetClient()->jid()) &&
      from != Jid(GetClient()->jid().domain()))
    return false;

  QueueStanza(stanza);
  return true;
}

int JingleInfoTask::ProcessStart() {
  std::vector<std::string> relay_hosts;
  std::vector<rtc::SocketAddress> stun_hosts;
  std::string relay_token;
  const XmlElement* stanza = NextStanza();
  if (stanza == NULL)
    return STATE_BLOCKED;
  const XmlElement* query = stanza->FirstNamed(QN_JINGLE_INFO_QUERY);
  if (query == NULL)
    return STATE_START;
  const XmlElement* stun = query->FirstNamed(QN_JINGLE_INFO_STUN);
  if (stun) {
    for (const XmlElement* server = stun->FirstNamed(QN_JINGLE_INFO_SERVER);
         server != NULL; server = server->NextNamed(QN_JINGLE_INFO_SERVER)) {
      std::string host = server->Attr(QN_JINGLE_INFO_HOST);
      std::string port = server->Attr(QN_JINGLE_INFO_UDP);
      if (host != STR_EMPTY && host != STR_EMPTY) {
        stun_hosts.push_back(rtc::SocketAddress(host, atoi(port.c_str())));
      }
    }
  }

  const XmlElement* relay = query->FirstNamed(QN_JINGLE_INFO_RELAY);
  if (relay) {
    relay_token = relay->TextNamed(QN_JINGLE_INFO_TOKEN);
    for (const XmlElement* server = relay->FirstNamed(QN_JINGLE_INFO_SERVER);
         server != NULL; server = server->NextNamed(QN_JINGLE_INFO_SERVER)) {
      std::string host = server->Attr(QN_JINGLE_INFO_HOST);
      if (host != STR_EMPTY) {
        relay_hosts.push_back(host);
      }
    }
  }
  SignalJingleInfo(relay_token, relay_hosts, stun_hosts);
  return STATE_START;
}
}
