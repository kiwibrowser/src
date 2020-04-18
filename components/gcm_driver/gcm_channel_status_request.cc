// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_channel_status_request.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/gcm_driver/gcm_backoff_policy.h"
#include "components/sync/protocol/experiment_status.pb.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

namespace gcm {

namespace {

const char kRequestContentType[] = "application/octet-stream";
const char kGCMChannelTag[] = "gcm_channel";
const int kDefaultPollIntervalSeconds = 60 * 60;  // 60 minutes.
const int kMinPollIntervalSeconds = 30 * 60;  // 30 minutes.

}  // namespace

GCMChannelStatusRequest::GCMChannelStatusRequest(
    const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
    const std::string& channel_status_request_url,
    const std::string& user_agent,
    const GCMChannelStatusRequestCallback& callback)
    : request_context_getter_(request_context_getter),
      channel_status_request_url_(channel_status_request_url),
      user_agent_(user_agent),
      callback_(callback),
      backoff_entry_(&(GetGCMBackoffPolicy())),
      weak_ptr_factory_(this) {
}

GCMChannelStatusRequest::~GCMChannelStatusRequest() {
}

// static
int GCMChannelStatusRequest::default_poll_interval_seconds() {
  return kDefaultPollIntervalSeconds;
}

// static
int GCMChannelStatusRequest::min_poll_interval_seconds() {
  return kMinPollIntervalSeconds;
}

void GCMChannelStatusRequest::Start() {
  DCHECK(!url_fetcher_);

  GURL request_url(channel_status_request_url_);

  sync_pb::ExperimentStatusRequest proto_data;
  proto_data.add_experiment_name(kGCMChannelTag);
  std::string upload_data;
  if (!proto_data.SerializeToString(&upload_data)) {
     NOTREACHED();
  }

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("gcm_channel_status_request", R"(
        semantics {
          sender: "GCM Driver"
          description:
            "Google Chrome interacts with Google Cloud Messaging to receive "
            "push messages for various browser features, as well as on behalf "
            "of websites and extensions. The channel status request "
            "periodically confirms with Google servers whether the feature "
            "should be enabled."
          trigger:
            "Periodically when Chrome has established an active Google Cloud "
            "Messaging subscription. The first request will be issued a minute "
            "after the first subscription activates. Subsequent requests will "
            "be issued each hour with a jitter of 15 minutes. Google can "
            "adjust this interval when it deems necessary."
          data:
            "A user agent string containing the Chrome version, channel and "
            "platform will be sent to the server. No user identifier is sent "
            "along with the request."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Support for interacting with Google Cloud Messaging is enabled by "
            "default, and there is no configuration option to completely "
            "disable it. Websites wishing to receive push messages must "
            "acquire express permission from the user for the 'Notification' "
            "permission."
          policy_exception_justification:
            "Not implemented, considered not useful."
        })");

  url_fetcher_ = net::URLFetcher::Create(request_url, net::URLFetcher::POST,
                                         this, traffic_annotation);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      url_fetcher_.get(), data_use_measurement::DataUseUserData::GCM_DRIVER);
  url_fetcher_->SetRequestContext(request_context_getter_.get());
  url_fetcher_->AddExtraRequestHeader("User-Agent: " + user_agent_);
  url_fetcher_->SetUploadData(kRequestContentType, upload_data);
  url_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SAVE_COOKIES);
  url_fetcher_->Start();
}

void GCMChannelStatusRequest::OnURLFetchComplete(
    const net::URLFetcher* source) {
  if (ParseResponse(source))
    return;

  RetryWithBackoff(true);
}

bool GCMChannelStatusRequest::ParseResponse(const net::URLFetcher* source) {
  if (!source->GetStatus().is_success()) {
    LOG(ERROR) << "GCM channel request failed.";
    return false;
  }

  if (source->GetResponseCode() != net::HTTP_OK) {
    LOG(ERROR) << "GCM channel request failed. HTTP status: "
               << source->GetResponseCode();
    return false;
  }

  std::string response_string;
  if (!source->GetResponseAsString(&response_string)) {
    LOG(ERROR) << "GCM channel response failed to be retrieved.";
    return false;
  }

  // Empty response means to keep the existing values.
  if (response_string.empty()) {
    callback_.Run(false, false, 0);
    return true;
  }

  sync_pb::ExperimentStatusResponse response_proto;
  if (!response_proto.ParseFromString(response_string)) {
    LOG(ERROR) << "GCM channel response failed to be parsed as proto.";
    return false;
  }

  bool enabled = true;
  if (response_proto.experiment_size() == 1 &&
      response_proto.experiment(0).has_gcm_channel() &&
      response_proto.experiment(0).gcm_channel().has_enabled()) {
    enabled = response_proto.experiment(0).gcm_channel().enabled();
  }

  int poll_interval_seconds;
  if (response_proto.has_poll_interval_seconds())
    poll_interval_seconds = response_proto.poll_interval_seconds();
  else
    poll_interval_seconds = kDefaultPollIntervalSeconds;
  if (poll_interval_seconds < kMinPollIntervalSeconds)
    poll_interval_seconds = kMinPollIntervalSeconds;

  callback_.Run(true, enabled, poll_interval_seconds);

  return true;
}

void GCMChannelStatusRequest::RetryWithBackoff(bool update_backoff) {
  if (update_backoff) {
    url_fetcher_.reset();
    backoff_entry_.InformOfRequest(false);
  }

  if (backoff_entry_.ShouldRejectRequest()) {
    DVLOG(1) << "Delaying GCM channel request for "
             << backoff_entry_.GetTimeUntilRelease().InMilliseconds()
             << " ms.";
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::Bind(&GCMChannelStatusRequest::RetryWithBackoff,
                              weak_ptr_factory_.GetWeakPtr(), false),
        backoff_entry_.GetTimeUntilRelease());
    return;
  }

  Start();
}

}  // namespace gcm
