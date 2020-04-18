// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/url_fetcher.h"

#include "base/bind.h"
#include "content/common/net/url_request_user_data.h"
#include "net/url_request/url_fetcher.h"

namespace content {

namespace {

std::unique_ptr<base::SupportsUserData::Data> CreateURLRequestUserData(
    int render_process_id,
    int render_frame_id) {
  return std::make_unique<URLRequestUserData>(render_process_id,
                                              render_frame_id);
}

}  // namespace

void AssociateURLFetcherWithRenderFrame(
    net::URLFetcher* url_fetcher,
    const base::Optional<url::Origin>& initiator,
    int render_process_id,
    int render_frame_id) {
  url_fetcher->SetInitiator(initiator);
  url_fetcher->SetURLRequestUserData(
      URLRequestUserData::kUserDataKey,
      base::Bind(&CreateURLRequestUserData, render_process_id,
                 render_frame_id));
}

}  // namespace content
