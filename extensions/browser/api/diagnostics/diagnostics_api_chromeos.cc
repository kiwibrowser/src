// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/diagnostics/diagnostics_api.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"

namespace extensions {

namespace {

const char kCount[] = "count";
const char kDefaultCount[] = "1";
const char kTTL[] = "ttl";
const char kTimeout[] = "timeout";
const char kSize[] = "size";

typedef base::Callback<void(
    DiagnosticsSendPacketFunction::SendPacketResultCode result_code,
    const std::string& ip,
    double latency)> SendPacketCallback;

bool ParseResult(const std::string& status, std::string* ip, double* latency) {
  // Parses the result and returns IP and latency.
  std::unique_ptr<base::Value> parsed_value(base::JSONReader::Read(status));
  if (!parsed_value)
    return false;

  base::DictionaryValue* result = NULL;
  if (!parsed_value->GetAsDictionary(&result) || result->size() != 1)
    return false;

  // Returns the first item.
  base::DictionaryValue::Iterator iterator(*result);

  const base::DictionaryValue* info;
  if (!iterator.value().GetAsDictionary(&info))
    return false;

  if (!info->GetDouble("avg", latency))
    return false;

  *ip = iterator.key();
  return true;
}

void OnTestICMPCompleted(const SendPacketCallback& callback,
                         bool succeeded,
                         const std::string& status) {
  std::string ip;
  double latency;
  if (!succeeded || !ParseResult(status, &ip, &latency)) {
    callback.Run(DiagnosticsSendPacketFunction::SEND_PACKET_FAILED, "", 0.0);
  } else {
    callback.Run(DiagnosticsSendPacketFunction::SEND_PACKET_OK, ip, latency);
  }
}

}  // namespace

void DiagnosticsSendPacketFunction::AsyncWorkStart() {
  std::map<std::string, std::string> config;
  config[kCount] = kDefaultCount;
  if (parameters_->options.ttl)
    config[kTTL] = base::IntToString(*parameters_->options.ttl);
  if (parameters_->options.timeout)
    config[kTimeout] = base::IntToString(*parameters_->options.timeout);
  if (parameters_->options.size)
    config[kSize] = base::IntToString(*parameters_->options.size);

  chromeos::DBusThreadManager::Get()
      ->GetDebugDaemonClient()
      ->TestICMPWithOptions(
          parameters_->options.ip, config,
          base::Bind(
              OnTestICMPCompleted,
              base::Bind(&DiagnosticsSendPacketFunction::OnCompleted, this)));
}

}  // namespace extensions
