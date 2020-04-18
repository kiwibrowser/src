/*
 *  Copyright 2010 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef THIRD_PARTY_XMPP_JINGLEINFOTASK_H_
#define THIRD_PARTY_XMPP_JINGLEINFOTASK_H_

#include <vector>

#include "third_party/libjingle_xmpp/xmpp/xmppengine.h"
#include "third_party/libjingle_xmpp/xmpp/xmpptask.h"
#include "third_party/webrtc/rtc_base/sigslot.h"
#include "third_party/webrtc/p2p/client/httpportallocator.h"

namespace buzz {

class JingleInfoTask : public XmppTask {
 public:
  explicit JingleInfoTask(XmppTaskParentInterface* parent)
      : XmppTask(parent, XmppEngine::HL_TYPE) {}

  virtual int ProcessStart();
  void RefreshJingleInfoNow();

  sigslot::signal3<const std::string&,
                   const std::vector<std::string>&,
                   const std::vector<rtc::SocketAddress>&>
      SignalJingleInfo;

 protected:
  class JingleInfoGetTask;
  friend class JingleInfoGetTask;

  virtual bool HandleStanza(const XmlElement* stanza);
};
}

#endif  // THIRD_PARTY_XMPP_JINGLEINFOTASK_H_
