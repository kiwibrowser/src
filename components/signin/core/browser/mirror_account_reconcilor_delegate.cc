// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/mirror_account_reconcilor_delegate.h"

#include "base/logging.h"
#include "components/signin/core/browser/account_reconcilor.h"

namespace signin {

MirrorAccountReconcilorDelegate::MirrorAccountReconcilorDelegate(
    SigninManagerBase* signin_manager)
    : signin_manager_(signin_manager) {
  DCHECK(signin_manager_);
  signin_manager_->AddObserver(this);
}

MirrorAccountReconcilorDelegate::~MirrorAccountReconcilorDelegate() {
  signin_manager_->RemoveObserver(this);
}

bool MirrorAccountReconcilorDelegate::IsReconcileEnabled() const {
  return signin_manager_->IsAuthenticated();
}

bool MirrorAccountReconcilorDelegate::IsAccountConsistencyEnforced() const {
  return true;
}

bool MirrorAccountReconcilorDelegate::ShouldAbortReconcileIfPrimaryHasError()
    const {
  return true;
}

std::string MirrorAccountReconcilorDelegate::GetFirstGaiaAccountForReconcile(
    const std::vector<std::string>& chrome_accounts,
    const std::vector<gaia::ListedAccount>& gaia_accounts,
    const std::string& primary_account,
    bool first_execution) const {
  // Mirror only uses the primary account, and it is never empty.
  DCHECK(!primary_account.empty());
  DCHECK(base::ContainsValue(chrome_accounts, primary_account));
  return primary_account;
}

void MirrorAccountReconcilorDelegate::GoogleSigninSucceeded(
    const std::string& account_id,
    const std::string& username) {
  reconcilor()->EnableReconcile();
}

void MirrorAccountReconcilorDelegate::GoogleSignedOut(
    const std::string& account_id,
    const std::string& username) {
  reconcilor()->DisableReconcile(true /* logout_all_gaia_accounts */);
}

}  // namespace signin
