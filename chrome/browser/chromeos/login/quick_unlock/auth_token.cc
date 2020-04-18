// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/quick_unlock/auth_token.h"

#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/login/auth/user_context.h"

namespace chromeos {
namespace quick_unlock {

const int AuthToken::kTokenExpirationSeconds = 5 * 60;

AuthToken::AuthToken(const chromeos::UserContext& user_context)
    : identifier_(base::UnguessableToken::Create()),
      user_context_(std::make_unique<chromeos::UserContext>(user_context)),
      weak_factory_(this) {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&AuthToken::Reset, weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(kTokenExpirationSeconds));
}

AuthToken::~AuthToken() = default;

base::Optional<std::string> AuthToken::Identifier() {
  if (!user_context_)
    return base::nullopt;
  return identifier_.ToString();
}

void AuthToken::ResetForTest() {
  Reset();
}

void AuthToken::Reset() {
  if (user_context_)
    user_context_->ClearSecrets();
  user_context_.reset();
}

}  // namespace quick_unlock
}  // namespace chromeos
