// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/gcd_rest_client.h"

#include <stdint.h>

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_clock.h"
#include "base/values.h"
#include "net/url_request/url_fetcher.h"
#include "remoting/base/logging.h"

namespace remoting {

GcdRestClient::GcdRestClient(const std::string& gcd_base_url,
                             const std::string& gcd_device_id,
                             const scoped_refptr<net::URLRequestContextGetter>&
                                 url_request_context_getter,
                             OAuthTokenGetter* token_getter)
    : gcd_base_url_(gcd_base_url),
      gcd_device_id_(gcd_device_id),
      url_request_context_getter_(url_request_context_getter),
      token_getter_(token_getter),
      clock_(base::DefaultClock::GetInstance()) {}

GcdRestClient::~GcdRestClient() = default;

void GcdRestClient::PatchState(
    std::unique_ptr<base::DictionaryValue> patch_details,
    const GcdRestClient::ResultCallback& callback) {
  DCHECK(!HasPendingRequest());

  // Construct a status update message in the format GCD expects.  The
  // message looks like this, where "..." is filled in from
  // |patch_details|:
  //
  // {
  //   requestTimeMs: T,
  //   patches: [{
  //     timeMs: T,
  //     patch: {...}
  //   }]
  // }
  //
  // Note that |now| is deliberately using a double to hold an integer
  // value because |DictionaryValue| doesn't support int64_t values, and
  // GCD doesn't accept fractional values.
  double now = clock_->Now().ToJavaTime();
  std::unique_ptr<base::DictionaryValue> patch_dict(new base::DictionaryValue);
  patch_dict->SetDouble("requestTimeMs", now);
  std::unique_ptr<base::ListValue> patch_list(new base::ListValue);
  std::unique_ptr<base::DictionaryValue> patch_item(new base::DictionaryValue);
  patch_item->Set("patch", std::move(patch_details));
  patch_item->SetDouble("timeMs", now);
  patch_list->Append(std::move(patch_item));
  patch_dict->Set("patches", std::move(patch_list));

  // Stringify the message.
  std::string patch_string;
  if (!base::JSONWriter::Write(*patch_dict, &patch_string)) {
    LOG(ERROR) << "Error building GCD device state patch.";
    callback.Run(OTHER_ERROR);
    return;
  }
  DLOG(INFO) << "sending state patch: " << patch_string;

  std::string url =
      gcd_base_url_ + "/devices/" + gcd_device_id_ + "/patchState";

  // Prepare an HTTP request to issue once an auth token is available.
  callback_ = callback;
  url_fetcher_ =
      net::URLFetcher::Create(GURL(url), net::URLFetcher::POST, this);
  url_fetcher_->SetUploadData("application/json", patch_string);
  if (url_request_context_getter_) {
    url_fetcher_->SetRequestContext(url_request_context_getter_.get());
  }

  token_getter_->CallWithToken(
      base::Bind(&GcdRestClient::OnTokenReceived, base::Unretained(this)));
}

void GcdRestClient::SetClockForTest(base::Clock* clock) {
  clock_ = clock;
}

void GcdRestClient::OnTokenReceived(OAuthTokenGetter::Status status,
                                    const std::string& user_email,
                                    const std::string& access_token) {
  DCHECK(HasPendingRequest());

  if (status != OAuthTokenGetter::SUCCESS) {
    LOG(ERROR) << "Error getting OAuth token for GCD request: "
               << url_fetcher_->GetOriginalURL();
    if (status == OAuthTokenGetter::NETWORK_ERROR) {
      FinishCurrentRequest(NETWORK_ERROR);
    } else {
      FinishCurrentRequest(OTHER_ERROR);
    }
    return;
  }

  url_fetcher_->SetExtraRequestHeaders(
      "Authorization: Bearer " + access_token);
  url_fetcher_->Start();
}

void GcdRestClient::FinishCurrentRequest(Result result) {
  DCHECK(HasPendingRequest());
  url_fetcher_.reset();
  base::ResetAndReturn(&callback_).Run(result);
}

void GcdRestClient::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(HasPendingRequest());

  const GURL& request_url = url_fetcher_->GetOriginalURL();
  Result status = OTHER_ERROR;
  int response = source->GetResponseCode();
  if (response >= 200 && response < 300) {
    DLOG(INFO) << "GCD request succeeded:" << request_url;
    status = SUCCESS;
  } else if (response == 404) {
    LOG(WARNING) << "Host not found (" << response
                 << ") fetching URL: " << request_url;
    status = NO_SUCH_HOST;
  } else if (response == 0) {
    LOG(ERROR) << "Network error (" << response
               << ") fetching URL: " << request_url;
    status = NETWORK_ERROR;
  } else {
    LOG(ERROR) << "Error (" << response << ") fetching URL: " << request_url;
  }

  FinishCurrentRequest(status);
}

}  // namespace remoting
