// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/base/telemetry_log_writer.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace remoting {

const int kMaxSendAttempts = 5;
constexpr char kAuthorizationHeaderPrefix[] = "Authorization:Bearer ";

TelemetryLogWriter::TelemetryLogWriter(
    const std::string& telemetry_base_url,
    std::unique_ptr<UrlRequestFactory> request_factory,
    std::unique_ptr<OAuthTokenGetter> token_getter)
    : telemetry_base_url_(telemetry_base_url),
      request_factory_(std::move(request_factory)),
      token_getter_(std::move(token_getter)) {
  DETACH_FROM_THREAD(thread_checker_);
  DCHECK(request_factory_);
  DCHECK(token_getter_);
}

TelemetryLogWriter::~TelemetryLogWriter() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void TelemetryLogWriter::Log(const ChromotingEvent& entry) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  pending_entries_.push_back(entry);
  SendPendingEntries();
}

void TelemetryLogWriter::SendPendingEntries() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!sending_entries_.empty() || pending_entries_.empty()) {
    return;
  }

  std::unique_ptr<base::ListValue> events(new base::ListValue());
  while (!pending_entries_.empty()) {
    ChromotingEvent& entry = pending_entries_.front();
    DCHECK(entry.IsDataValid());
    events->Append(entry.CopyDictionaryValue());
    entry.IncrementSendAttempts();
    sending_entries_.push_back(std::move(entry));
    pending_entries_.pop_front();
  }
  base::DictionaryValue log_dictionary;
  log_dictionary.Set("event", std::move(events));

  std::string json;
  JSONStringValueSerializer serializer(&json);
  if (!serializer.Serialize(log_dictionary)) {
    LOG(ERROR) << "Failed to serialize log to JSON.";
    return;
  }
  token_getter_->CallWithToken(base::BindRepeating(
      &TelemetryLogWriter::PostJsonToServer, base::Unretained(this), json));
}

void TelemetryLogWriter::PostJsonToServer(const std::string& json,
                                          OAuthTokenGetter::Status status,
                                          const std::string& user_email,
                                          const std::string& access_token) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!request_);
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("CRD_telemetry_log", R"(
        semantics {
          sender: "Chrome Remote Desktop"
          description: "Telemetry logs for Chrome Remote Desktop."
          trigger:
            "These requests are sent periodically when a session is connected, "
            "i.e. CRD app is open and is connected to a host."
          data:
            "Anonymous usage statistics. Please see https://cs.chromium.org/"
            "chromium/src/remoting/base/chromoting_event.h for more details."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings. You can block Chrome "
            "Remote Desktop as specified here: "
            "https://support.google.com/chrome/?p=remote_desktop"
          policy_exception_justification:
            "The product is shipped separately from Chromium, except on Chrome "
            "OS."
        })");
  request_ = request_factory_->CreateUrlRequest(
      UrlRequest::Type::POST, telemetry_base_url_, traffic_annotation);
  request_->AddHeader(kAuthorizationHeaderPrefix + access_token);

  VLOG(1) << "Posting log to telemetry server: " << json;

  request_->SetPostData("application/json", json);
  request_->Start(base::BindRepeating(&TelemetryLogWriter::OnSendLogResult,
                                      base::Unretained(this)));
}

void TelemetryLogWriter::OnSendLogResult(
    const remoting::UrlRequest::Result& result) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(request_);
  if (!result.success || result.status != net::HTTP_OK) {
    LOG(WARNING) << "Error occur when sending logs to the telemetry server, "
                 << "status: " << result.status;
    VLOG(1) << "Response body: \n"
            << "body: " << result.response_body;

    // Reverse iterating + push_front in order to restore the order of logs.
    for (auto i = sending_entries_.rbegin(); i < sending_entries_.rend(); i++) {
      if (i->send_attempts() >= kMaxSendAttempts) {
        break;
      }
      pending_entries_.push_front(std::move(*i));
    }
  } else {
    VLOG(1) << "Successfully sent " << sending_entries_.size()
            << " log(s) to telemetry server.";
  }
  sending_entries_.clear();
  request_.reset();  // This may also destroy the result.
  SendPendingEntries();
}

}  // namespace remoting
