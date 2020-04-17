// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/content/view_impl.h"

#include "base/bind.h"
#include "services/content/service.h"
#include "services/content/service_delegate.h"
#include "services/content/view_delegate.h"

namespace content {

ViewImpl::ViewImpl(Service* service,
                   mojom::ViewParamsPtr params,
                   mojom::ViewRequest request,
                   mojom::ViewClientPtr client)
    : service_(service),
      binding_(this, std::move(request)),
      client_(std::move(client)),
      delegate_(service_->delegate()->CreateViewDelegate(client_.get())) {
  binding_.set_connection_error_handler(base::BindRepeating(
      &Service::RemoveView, base::Unretained(service_), this));
}

ViewImpl::~ViewImpl() = default;

void ViewImpl::Navigate(const GURL& url) {
  // Ignore non-HTTP/HTTPS requests for now.
  if (!url.SchemeIsHTTPOrHTTPS())
    return;

  delegate_->Navigate(url);
}

}  // namespace content
