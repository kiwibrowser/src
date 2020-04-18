/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_LIBJINGLE_XMPP_SASLPLAINMECHANISM_H_
#define WEBRTC_LIBJINGLE_XMPP_SASLPLAINMECHANISM_H_

#include "third_party/libjingle_xmpp/xmpp/saslmechanism.h"

namespace buzz {

class SaslPlainMechanism : public SaslMechanism {

public:
  SaslPlainMechanism(const buzz::Jid user_jid, const std::string & password) :
    user_jid_(user_jid), password_(password) {}

  virtual std::string GetMechanismName() { return "PLAIN"; }

  virtual XmlElement * StartSaslAuth() {
    // send initial request
    XmlElement * el = new XmlElement(QN_SASL_AUTH, true);
    el->AddAttr(QN_MECHANISM, "PLAIN");

    std::stringstream ss;
    ss.write("\0", 1);
    ss << user_jid_.node();
    ss.write("\0", 1);
    ss << password_;
    el->AddText(Base64EncodeFromArray(ss.str().data(), ss.str().length()));
    return el;
  }

private:
  Jid user_jid_;
  std::string password_;
};

}

#endif  // WEBRTC_LIBJINGLE_XMPP_SASLPLAINMECHANISM_H_
