// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/web_package_context_impl.h"

#include "content/browser/web_package/signed_exchange_handler.h"
#include "content/public/browser/browser_thread.h"

namespace content {

WebPackageContextImpl::WebPackageContextImpl() = default;

WebPackageContextImpl::~WebPackageContextImpl() = default;

void WebPackageContextImpl::SetSignedExchangeVerificationTimeForTesting(
    base::Optional<base::Time> time) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  SignedExchangeHandler::SetVerificationTimeForTesting(time);
}

}  // namespace content
