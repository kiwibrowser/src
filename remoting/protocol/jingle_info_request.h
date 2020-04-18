// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_JINGLE_INFO_REQUEST_H_
#define REMOTING_PROTOCOL_JINGLE_INFO_REQUEST_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "remoting/protocol/ice_config_request.h"
#include "remoting/signaling/iq_sender.h"

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

class SignalStrategy;

namespace protocol {

// IceConfigRequest that fetches IceConfig from Google Talk servers using HTTP.
class JingleInfoRequest : public IceConfigRequest {
 public:
  explicit JingleInfoRequest(SignalStrategy* signal_strategy);
  ~JingleInfoRequest() override;

  // IceConfigRequest interface.
  void Send(const OnIceConfigCallback& callback) override;

 private:
  void OnResponse(IqRequest* request, const buzz::XmlElement* stanza);

  IqSender iq_sender_;
  std::unique_ptr<IqRequest> request_;
  OnIceConfigCallback on_ice_config_callback_;

  DISALLOW_COPY_AND_ASSIGN(JingleInfoRequest);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_JINGLE_INFO_REQUEST_H_
