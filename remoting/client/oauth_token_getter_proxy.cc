// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/oauth_token_getter_proxy.h"

#include "base/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "base/threading/thread_task_runner_handle.h"

namespace remoting {

namespace {

void ResolveCallback(
    const OAuthTokenGetter::TokenCallback& on_access_token,
    scoped_refptr<base::SingleThreadTaskRunner> original_task_runner,
    OAuthTokenGetter::Status status,
    const std::string& user_email,
    const std::string& access_token) {
  if (!original_task_runner->BelongsToCurrentThread()) {
    original_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(on_access_token, status, user_email, access_token));
    return;
  }

  on_access_token.Run(status, user_email, access_token);
}

}  // namespace

OAuthTokenGetterProxy::OAuthTokenGetterProxy(
    base::WeakPtr<OAuthTokenGetter> token_getter,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : token_getter_(token_getter), task_runner_(task_runner) {}

OAuthTokenGetterProxy::~OAuthTokenGetterProxy() {}

void OAuthTokenGetterProxy::CallWithToken(
    const OAuthTokenGetter::TokenCallback& on_access_token) {
  if (!task_runner_->BelongsToCurrentThread()) {
    auto task_runner_to_reply = base::ThreadTaskRunnerHandle::Get();

    // TODO(crbug.com/824488): Change to base::BindOnce().
    auto reply_callback = base::BindRepeating(&ResolveCallback, on_access_token,
                                              task_runner_to_reply);
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(&OAuthTokenGetter::CallWithToken,
                                          token_getter_, reply_callback));
    return;
  }

  if (token_getter_) {
    token_getter_->CallWithToken(on_access_token);
  }
}

void OAuthTokenGetterProxy::InvalidateCache() {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&OAuthTokenGetter::InvalidateCache, token_getter_));
    return;
  }

  if (token_getter_) {
    token_getter_->InvalidateCache();
  }
}

}  // namespace remoting
