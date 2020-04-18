// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/hsts_query.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/task_runner_util.h"
#include "net/url_request/url_request_context.h"
#include "url/gurl.h"

namespace password_manager {

namespace {

bool IsHSTSActiveForHostAndRequestContext(
    const GURL& origin,
    const scoped_refptr<net::URLRequestContextGetter>& request_context) {
  if (!origin.is_valid())
    return false;

  net::TransportSecurityState* security_state =
      request_context->GetURLRequestContext()->transport_security_state();

  if (!security_state)
    return false;

  return security_state->ShouldUpgradeToSSL(origin.host());
}

}  // namespace

void PostHSTSQueryForHostAndRequestContext(
    const GURL& origin,
    const scoped_refptr<net::URLRequestContextGetter>& request_context,
    const HSTSCallback& callback) {
  base::PostTaskAndReplyWithResult(
      request_context->GetNetworkTaskRunner().get(), FROM_HERE,
      base::Bind(&IsHSTSActiveForHostAndRequestContext, origin,
                 request_context),
      callback);
}

}  // namespace password_manager
