// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/signaling_address.h"

#include "base/base64.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "remoting/base/name_value_map.h"
#include "remoting/base/remoting_bot.h"
#include "remoting/base/service_urls.h"
#include "remoting/signaling/jid_util.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"

namespace remoting {

namespace {

const char kJingleNamespace[] = "urn:xmpp:jingle:1";

// Represents the XML attrbute names for the various address fields in the
// iq stanza.
enum class Field { JID, CHANNEL, ENDPOINT_ID };

const NameMapElement<SignalingAddress::Channel> kChannelTypes[] = {
    {SignalingAddress::Channel::LCS, "lcs"},
    {SignalingAddress::Channel::XMPP, "xmpp"},
};

buzz::QName GetQNameByField(Field attr, SignalingAddress::Direction direction) {
  const char* attribute_name = nullptr;
  switch (attr) {
    case Field::JID:
      attribute_name = (direction == SignalingAddress::FROM) ? "from" : "to";
      break;
    case Field::ENDPOINT_ID:
      attribute_name = (direction == SignalingAddress::FROM)
                           ? "from-endpoint-id"
                           : "to-endpoint-id";
      break;
    case Field::CHANNEL:
      attribute_name =
          (direction == SignalingAddress::FROM) ? "from-channel" : "to-channel";
      break;
  }
  return buzz::QName("", attribute_name);
}

SignalingAddress::Channel GetChannelType(std::string address) {
  std::string bare_jid;
  std::string resource;
  if (SplitJidResource(address, &bare_jid, &resource) &&
      resource.find("chromoting_lcs_") == 0) {
    return SignalingAddress::Channel::LCS;
  }
  return SignalingAddress::Channel::XMPP;
}

}  // namespace

SignalingAddress::SignalingAddress()
    : channel_(SignalingAddress::Channel::XMPP) {}

SignalingAddress::SignalingAddress(const std::string& address) {
  channel_ = GetChannelType(address);
  switch (channel_) {
    case SignalingAddress::Channel::XMPP:
      jid_ = NormalizeJid(address);
      break;
    case SignalingAddress::Channel::LCS:
      endpoint_id_ = NormalizeJid(address);
      jid_ = remoting::ServiceUrls::GetInstance()->directory_bot_jid();
      break;
    default:
      NOTREACHED();
  }

  DCHECK(!jid_.empty()) << "Missing JID.";
  DCHECK(endpoint_id_.empty() != (channel_ == Channel::LCS))
      << "EndpointId should be set if and only if the Channel is LCS";
}

SignalingAddress::SignalingAddress(const std::string& jid,
                                   const std::string& endpoint_id,
                                   Channel channel)
    : jid_(NormalizeJid(jid)),
      endpoint_id_(NormalizeJid(endpoint_id)),
      channel_(channel) {
  DCHECK(!jid.empty());
}

bool SignalingAddress::operator==(const SignalingAddress& other) const {
  return (other.jid_ == jid_) && (other.endpoint_id_ == endpoint_id_) &&
         (other.channel_ == channel_);
}

bool SignalingAddress::operator!=(const SignalingAddress& other) const {
  return !(*this == other);
}

SignalingAddress SignalingAddress::Parse(const buzz::XmlElement* iq,
                                         SignalingAddress::Direction direction,
                                         std::string* error) {
  std::string jid(iq->Attr(GetQNameByField(Field::JID, direction)));
  if (jid.empty()) {
    return SignalingAddress();
  }

  const buzz::XmlElement* jingle =
      iq->FirstNamed(buzz::QName(kJingleNamespace, "jingle"));

  if (!jingle) {
    return SignalingAddress(jid);
  }

  std::string type(iq->Attr(buzz::QName(std::string(), "type")));
  // For error IQs, invert the direction as the jingle node represents the
  // original request.
  if (type == "error") {
    direction = (direction == FROM) ? TO : FROM;
  }

  std::string endpoint_id(
      jingle->Attr(GetQNameByField(Field::ENDPOINT_ID, direction)));
  std::string channel_str(
      jingle->Attr(GetQNameByField(Field::CHANNEL, direction)));
  SignalingAddress::Channel channel;

  if (channel_str.empty()) {
    channel = SignalingAddress::Channel::XMPP;
  } else if (!NameToValue(kChannelTypes, channel_str, &channel)) {
    *error = "Unknown channel: " + channel_str;
    return SignalingAddress();
  }

  bool is_lcs = (channel == SignalingAddress::Channel::LCS);

  if (is_lcs == endpoint_id.empty()) {
    *error = (is_lcs ? "Missing |endpoint-id| for LCS channel"
                     : "|endpoint_id| should be empty for XMPP channel");
    return SignalingAddress();
  }

  if (direction == FROM && is_lcs && !IsValidBotJid(jid)) {
    *error = "Reject LCS message from untrusted sender: " + jid;
    return SignalingAddress();
  }

  return SignalingAddress(jid, endpoint_id, channel);
}

void SignalingAddress::SetInMessage(buzz::XmlElement* iq,
                                    Direction direction) const {
  DCHECK(!empty()) << "Signaling Address is empty";

  if (direction == FROM) {
    // The from JID is not used for routing but for sender identification on the
    // receiving end. Use the ID for specificity.
    iq->SetAttr(GetQNameByField(Field::JID, direction), id());
  } else {
    // The to JID is used for routing and must be a valid XMPP endpoint.  Always
    // use the XMPP JID.
    iq->SetAttr(GetQNameByField(Field::JID, direction), jid_);
  }

  // Do not tamper the routing-info in the jingle tag for error IQ's, as
  // it corresponds to the original message.
  std::string type(iq->Attr(buzz::QName(std::string(), "type")));
  if (type == "error") {
    return;
  }

  buzz::XmlElement* jingle =
      iq->FirstNamed(buzz::QName(kJingleNamespace, "jingle"));

  if (jingle) {
    // Start from a fresh slate regardless of the previous address format.
    jingle->ClearAttr(GetQNameByField(Field::CHANNEL, direction));
    jingle->ClearAttr(GetQNameByField(Field::ENDPOINT_ID, direction));

    // Only set the channel and endpoint_id in the LCS channel.
    if (channel_ == SignalingAddress::Channel::LCS) {
      jingle->AddAttr(GetQNameByField(Field::ENDPOINT_ID, direction),
                      endpoint_id_);
      jingle->AddAttr(GetQNameByField(Field::CHANNEL, direction),
                      ValueToName(kChannelTypes, channel_));
    }
  }
}

}  // namespace remoting
