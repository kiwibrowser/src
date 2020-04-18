// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/timezone/timezone_provider.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chromeos/geolocation/geoposition.h"
#include "net/url_request/url_request_context_getter.h"

namespace chromeos {

TimeZoneProvider::TimeZoneProvider(
    net::URLRequestContextGetter* url_context_getter,
    const GURL& url)
    : url_context_getter_(url_context_getter), url_(url) {
}

TimeZoneProvider::~TimeZoneProvider() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void TimeZoneProvider::RequestTimezone(
    const Geoposition& position,
    base::TimeDelta timeout,
    TimeZoneRequest::TimeZoneResponseCallback callback) {
  TimeZoneRequest* request(new TimeZoneRequest(
      url_context_getter_.get(), url_, position,timeout));
  requests_.push_back(base::WrapUnique(request));

  // TimeZoneProvider owns all requests. It is safe to pass unretained "this"
  // because destruction of TimeZoneProvider cancels all requests.
  TimeZoneRequest::TimeZoneResponseCallback callback_tmp(
      base::Bind(&TimeZoneProvider::OnTimezoneResponse,
                 base::Unretained(this),
                 request,
                 callback));
  request->MakeRequest(callback_tmp);
}

void TimeZoneProvider::OnTimezoneResponse(
    TimeZoneRequest* request,
    TimeZoneRequest::TimeZoneResponseCallback callback,
    std::unique_ptr<TimeZoneResponseData> timezone,
    bool server_error) {
  std::vector<std::unique_ptr<TimeZoneRequest>>::iterator position =
      std::find_if(requests_.begin(), requests_.end(),
                   [request](const std::unique_ptr<TimeZoneRequest>& req) {
                     return req.get() == request;
                   });
  DCHECK(position != requests_.end());
  if (position != requests_.end()) {
    std::swap(*position, *requests_.rbegin());
    requests_.resize(requests_.size() - 1);
  }

  callback.Run(std::move(timezone), server_error);
}

}  // namespace chromeos
