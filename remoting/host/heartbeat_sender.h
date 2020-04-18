// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_HEARTBEAT_SENDER_H_
#define REMOTING_HOST_HEARTBEAT_SENDER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"
#include "remoting/base/rsa_key_pair.h"
#include "remoting/signaling/signal_strategy.h"

#ifdef ERROR
#undef ERROR
#endif

namespace base {
class TimeDelta;
}  // namespace base

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

class RsaKeyPair;
class IqRequest;
class IqSender;

// HeartbeatSender periodically sends heartbeat stanzas to the Chromoting Bot.
// Each heartbeat stanza looks as follows:
//
// <iq type="set" to="remoting@bot.talk.google.com"
//     from="user@gmail.com/chromoting123123" id="5" xmlns="jabber:client">
//   <rem:heartbeat rem:hostid="a1ddb11e-8aef-11df-bccf-18a905b9cb5a"
//                  rem:sequence-id="456"
//                  xmlns:rem="google:remoting">
//     <rem:signature>.signature.</rem:signature>
//   </rem:heartbeat>
// </iq>
//
// Normally the heartbeat indicates that the host is healthy and ready to
// accept new connections from a client, but the rem:heartbeat xml element can
// optionally include a rem:host-offline-reason attribute, which indicates that
// the host cannot accept connections from the client (and might possibly be
// shutting down).  The value of the host-offline-reason attribute can be either
// a string from host_exit_codes.cc (i.e. "INVALID_HOST_CONFIGURATION" string)
// or one of kHostOfflineReasonXxx constants (i.e. "POLICY_READ_ERROR" string).
//
// The sequence-id attribute of the heartbeat is a zero-based incrementally
// increasing integer unique to each heartbeat from a single host.
// The Bot checks the value, and if it is incorrect, includes the
// correct value in the result stanza. The host should then send another
// heartbeat, with the correct sequence-id, and increment the sequence-id in
// susbequent heartbeats.
// The signature is a base-64 encoded SHA-1 hash, signed with the host's
// private RSA key. The message being signed is the full Jid concatenated with
// the sequence-id, separated by one space. For example, for the heartbeat
// stanza above, the message that is signed is
// "user@gmail.com/chromoting123123 456".
//
// The Bot sends the following result stanza in response to each successful
// heartbeat:
//
//  <iq type="set" from="remoting@bot.talk.google.com"
//      to="user@gmail.com/chromoting123123" id="5" xmlns="jabber:client">
//    <rem:heartbeat-result xmlns:rem="google:remoting">
//      <rem:set-interval>300</rem:set-interval>
//    </rem:heartbeat-result>
//  </iq>
//
// The set-interval tag is used to specify desired heartbeat interval
// in seconds. The heartbeat-result and the set-interval tags are
// optional. Host uses default heartbeat interval if it doesn't find
// set-interval tag in the result Iq stanza it receives from the
// server.
// If the heartbeat's sequence-id was incorrect, the Bot sends a result
// stanza of this form:
//
//  <iq type="set" from="remoting@bot.talk.google.com"
//      to="user@gmail.com/chromoting123123" id="5" xmlns="jabber:client">
//    <rem:heartbeat-result xmlns:rem="google:remoting">
//      <rem:expected-sequence-id>654</rem:expected-sequence-id>
//    </rem:heartbeat-result>
//  </iq>
class HeartbeatSender : public SignalStrategy::Listener {
 public:
  // |signal_strategy| and |delegate| must outlive this
  // object. Heartbeats will start when the supplied SignalStrategy
  // enters the CONNECTED state.
  //
  // |on_heartbeat_successful_callback| is invoked after the first successful
  // heartbeat.
  //
  // |on_unknown_host_id_error| is invoked when the host ID is permanently not
  // recognized by the server.
  HeartbeatSender(const base::Closure& on_heartbeat_successful_callback,
                  const base::Closure& on_unknown_host_id_error,
                  const std::string& host_id,
                  SignalStrategy* signal_strategy,
                  const scoped_refptr<const RsaKeyPair>& host_key_pair,
                  const std::string& directory_bot_jid);
  ~HeartbeatSender() override;

  // Sets host offline reason for future heartbeat stanzas, and initiates
  // sending a stanza right away.
  //
  // For discussion of allowed values for |host_offline_reason| argument,
  // please see the description of rem:host-offline-reason xml attribute in
  // the class-level comments above.
  //
  // |ack_callback| will be called once, when the bot acks receiving the
  // |host_offline_reason| or when |timeout| is reached.
  void SetHostOfflineReason(
      const std::string& host_offline_reason,
      const base::TimeDelta& timeout,
      const base::Callback<void(bool success)>& ack_callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(HeartbeatSenderTest, SetInterval);

  enum class HeartbeatResult {
    SUCCESS,
    SET_SEQUENCE_ID,
    INVALID_HOST_ID,
    TIMEOUT,
    ERROR,
  };

  // SignalStrategy::Listener interface.
  void OnSignalStrategyStateChange(SignalStrategy::State state) override;
  bool OnSignalStrategyIncomingStanza(const buzz::XmlElement* stanza) override;

  void SendHeartbeat();
  void OnResponse(IqRequest* request, const buzz::XmlElement* response);
  HeartbeatResult ProcessResponse(const buzz::XmlElement* response);

  // Handlers for host-offline-reason completion and timeout.
  void OnHostOfflineReasonTimeout();
  void OnHostOfflineReasonAck();

  // Helper methods used by DoSendStanza() to generate heartbeat stanzas.
  std::unique_ptr<buzz::XmlElement> CreateHeartbeatMessage();
  std::unique_ptr<buzz::XmlElement> CreateSignature();

  base::Closure on_heartbeat_successful_callback_;
  base::Closure on_unknown_host_id_error_;
  std::string host_id_;
  SignalStrategy* const signal_strategy_;
  scoped_refptr<const RsaKeyPair> host_key_pair_;
  std::string directory_bot_jid_;
  std::unique_ptr<IqSender> iq_sender_;
  std::unique_ptr<IqRequest> request_;

  base::TimeDelta interval_;
  base::OneShotTimer timer_;

  int failed_heartbeat_count_ = 0;

  int sequence_id_ = 0;
  bool heartbeat_succeeded_ = false;
  int timed_out_heartbeats_count_ = 0;

  // Fields to send and indicate completion of sending host-offline-reason.
  std::string host_offline_reason_;
  base::Callback<void(bool success)> host_offline_reason_ack_callback_;
  base::OneShotTimer host_offline_reason_timeout_timer_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(HeartbeatSender);
};

}  // namespace remoting

#endif  // REMOTING_HOST_HEARTBEAT_SENDER_H_
