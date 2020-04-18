// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_SIGNALING_SIGNALING_ADDRESS_H_
#define REMOTING_SIGNALING_SIGNALING_ADDRESS_H_

#include <string>

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

// Represents an address of a Chromoting endpoint and its routing channel.
class SignalingAddress {
 public:
  enum class Channel { LCS, XMPP };
  enum Direction { TO, FROM };
  // Creates an empty SignalingAddress.
  SignalingAddress();

  // Creates a SignalingAddress with |jid|, which can either be a valid
  // XMPP JID or an LCS address in a JID like format.
  explicit SignalingAddress(const std::string& jid);

  static SignalingAddress Parse(const buzz::XmlElement* iq,
                                Direction direction,
                                std::string* error);

  void SetInMessage(buzz::XmlElement* message, Direction direction) const;

  const std::string& jid() const { return jid_; }
  const std::string& endpoint_id() const { return endpoint_id_; }
  Channel channel() const { return channel_; }
  const std::string& id() const {
    return (channel_ == Channel::LCS) ? endpoint_id_ : jid_;
  }

  bool empty() const { return jid_.empty(); }

  bool operator==(const SignalingAddress& other) const;
  bool operator!=(const SignalingAddress& other) const;

 private:
  SignalingAddress(const std::string& jid,
                   const std::string& endpoint_id,
                   Channel channel);

  // Represents the |to| or |from| field in an IQ stanza.
  std::string jid_;

  // Represents the identifier of an endpoint. In  LCS, this is the LCS address
  // encoded in a JID like format.  In XMPP, it is empty.
  std::string endpoint_id_;

  Channel channel_;
};

}  // namespace remoting

#endif  // REMOTING_SIGNALING_SIGNALING_ADDRESS_H_
