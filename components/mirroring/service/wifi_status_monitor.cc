// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mirroring/service/wifi_status_monitor.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "components/mirroring/service/message_dispatcher.h"

namespace mirroring {

namespace {

// The interval to query the status.
constexpr base::TimeDelta kQueryInterval = base::TimeDelta::FromMinutes(2);

// The maximum number of recent status to be kept.
constexpr int kMaxRecords = 30;

}  // namespace

WifiStatusMonitor::WifiStatusMonitor(int32_t session_id,
                                     MessageDispatcher* message_dispatcher)
    : session_id_(session_id), message_dispatcher_(message_dispatcher) {
  DCHECK(message_dispatcher_);
  message_dispatcher_->Subscribe(
      ResponseType::STATUS_RESPONSE,
      base::BindRepeating(&WifiStatusMonitor::RecordStatus,
                          base::Unretained(this)));
  query_timer_.Start(FROM_HERE, kQueryInterval,
                     base::BindRepeating(&WifiStatusMonitor::QueryStatus,
                                         base::Unretained(this)));
  QueryStatus();
}

WifiStatusMonitor::~WifiStatusMonitor() {
  message_dispatcher_->Unsubscribe(ResponseType::STATUS_RESPONSE);
}

std::vector<WifiStatus> WifiStatusMonitor::GetRecentValues() {
  std::vector<WifiStatus> recent_status(recent_status_.begin(),
                                        recent_status_.end());
  recent_status_.clear();
  return recent_status;
}

void WifiStatusMonitor::QueryStatus() {
  base::Value query(base::Value::Type::DICTIONARY);
  query.SetKey("type", base::Value("GET_STATUS"));
  query.SetKey("sessionId", base::Value(session_id_));
  query.SetKey("seqNum", base::Value(message_dispatcher_->GetNextSeqNumber()));
  base::Value::ListStorage status;
  status.emplace_back(base::Value("wifiSnr"));
  status.emplace_back(base::Value("wifiSpeed"));
  query.SetKey("get_status", base::Value(status));
  CastMessage get_status_message;
  get_status_message.message_namespace = kWebRtcNamespace;
  const bool did_serialize_query =
      base::JSONWriter::Write(query, &get_status_message.json_format_data);
  DCHECK(did_serialize_query);
  message_dispatcher_->SendOutboundMessage(get_status_message);
}

void WifiStatusMonitor::RecordStatus(const ReceiverResponse& response) {
  if (!response.status || response.status->wifi_speed.size() != 4)
    return;
  if (recent_status_.size() == kMaxRecords)
    recent_status_.pop_front();
  WifiStatus received_status;
  received_status.snr = response.status->wifi_snr;
  // Only records the current speed.
  received_status.speed = response.status->wifi_speed[3];
  received_status.timestamp = base::Time::Now();
  recent_status_.emplace_back(received_status);
}

}  // namespace mirroring
