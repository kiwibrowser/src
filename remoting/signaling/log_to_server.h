// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_SIGNALING_LOG_TO_SERVER_H_
#define REMOTING_SIGNALING_LOG_TO_SERVER_H_

#include <map>
#include <string>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/sequence_checker.h"
#include "remoting/signaling/server_log_entry.h"
#include "remoting/signaling/signal_strategy.h"

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

class IqSender;

// LogToServer sends log entries to a server.
// The contents of the log entries are described in server_log_entry.cc.
// They do not contain any personally identifiable information.
class LogToServer : public SignalStrategy::Listener {
 public:
  LogToServer(ServerLogEntry::Mode mode,
              SignalStrategy* signal_strategy,
              const std::string& directory_bot_jid);
  ~LogToServer() override;

  // SignalStrategy::Listener interface.
  void OnSignalStrategyStateChange(SignalStrategy::State state) override;
  bool OnSignalStrategyIncomingStanza(const buzz::XmlElement* stanza) override;

  void Log(const ServerLogEntry& entry);

  ServerLogEntry::Mode mode() { return mode_; }

 private:
  void SendPendingEntries();

  ServerLogEntry::Mode mode_;
  SignalStrategy* signal_strategy_;
  std::unique_ptr<IqSender> iq_sender_;
  std::string directory_bot_jid_;

  base::circular_deque<ServerLogEntry> pending_entries_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(LogToServer);
};

}  // namespace remoting

#endif  // REMOTING_SIGNALING_LOG_TO_SERVER_H_
