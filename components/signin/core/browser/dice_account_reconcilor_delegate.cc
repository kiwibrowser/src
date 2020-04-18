// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/dice_account_reconcilor_delegate.h"

#include <vector>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_client.h"
#include "components/signin/core/browser/signin_pref_names.h"

namespace signin {

DiceAccountReconcilorDelegate::DiceAccountReconcilorDelegate(
    SigninClient* signin_client,
    AccountConsistencyMethod account_consistency)
    : signin_client_(signin_client), account_consistency_(account_consistency) {
  DCHECK(signin_client_);
}

bool DiceAccountReconcilorDelegate::IsReconcileEnabled() const {
  return DiceMethodGreaterOrEqual(
      account_consistency_, AccountConsistencyMethod::kDicePrepareMigration);
}

bool DiceAccountReconcilorDelegate::IsAccountConsistencyEnforced() const {
  return account_consistency_ == AccountConsistencyMethod::kDice;
}

// - On first execution, the candidates are examined in this order:
//   1. The primary account
//   2. The current first Gaia account
//   3. The last known first Gaia account
//   4. The first account in the token service
// - On subsequent executions, the order is:
//   1. The current first Gaia account
//   2. The primary account
//   3. The last known first Gaia account
//   4. The first account in the token service
std::string DiceAccountReconcilorDelegate::GetFirstGaiaAccountForReconcile(
    const std::vector<std::string>& chrome_accounts,
    const std::vector<gaia::ListedAccount>& gaia_accounts,
    const std::string& primary_account,
    bool first_execution) const {
  if (chrome_accounts.empty())
    return std::string();  // No Chrome account, log out.

  bool valid_primary_account =
      !primary_account.empty() &&
      base::ContainsValue(chrome_accounts, primary_account);

  if (gaia_accounts.empty()) {
    if (valid_primary_account)
      return primary_account;

    // Try the last known account. This happens when the cookies are cleared
    // while Sync is disabled.
    if (base::ContainsValue(chrome_accounts, last_known_first_account_))
      return last_known_first_account_;

    // As a last resort, use the first Chrome account.
    return chrome_accounts[0];
  }

  const std::string& first_gaia_account = gaia_accounts[0].id;
  bool first_gaia_account_is_valid =
      gaia_accounts[0].valid &&
      base::ContainsValue(chrome_accounts, first_gaia_account);

  if (!first_gaia_account_is_valid && (primary_account == first_gaia_account)) {
    // The primary account is also the first Gaia account, and is invalid.
    // Logout everything.
    return std::string();
  }

  if (first_execution) {
    // On first execution, try the primary account, and then the first Gaia
    // account.
    if (valid_primary_account)
      return primary_account;
    if (first_gaia_account_is_valid)
      return first_gaia_account;
    // As a last resort, use the first Chrome account.
    return chrome_accounts[0];
  }

  // While Chrome is running, try the first Gaia account, and then the
  // primary account.
  if (first_gaia_account_is_valid)
    return first_gaia_account;
  if (valid_primary_account)
    return primary_account;

  // Changing the first Gaia account while Chrome is running would be
  // confusing for the user. Logout everything.
  return std::string();
}

AccountReconcilorDelegate::RevokeTokenOption
DiceAccountReconcilorDelegate::ShouldRevokeSecondaryTokensBeforeReconcile(
    const std::vector<gaia::ListedAccount>& gaia_accounts) {
  // During the Dice migration step, before Dice is actually enabled, chrome
  // tokens must be cleared when the cookies are cleared.
  if (DiceMethodGreaterOrEqual(
          account_consistency_,
          AccountConsistencyMethod::kDicePrepareMigration) &&
      (account_consistency_ != AccountConsistencyMethod::kDice) &&
      gaia_accounts.empty()) {
    return RevokeTokenOption::kRevoke;
  }

  return (account_consistency_ == AccountConsistencyMethod::kDice)
             ? RevokeTokenOption::kRevokeIfInError
             : RevokeTokenOption::kDoNotRevoke;
}

void DiceAccountReconcilorDelegate::OnReconcileFinished(
    const std::string& first_account,
    bool reconcile_is_noop) {
  last_known_first_account_ = first_account;

  // Migration happens on startup if the last reconcile was a no-op and the
  // refresh tokens are Dice-compatible.
  if (DiceMethodGreaterOrEqual(
          account_consistency_,
          AccountConsistencyMethod::kDicePrepareMigration)) {
    signin_client_->SetReadyForDiceMigration(
        reconcile_is_noop && signin_client_->GetPrefs()->GetBoolean(
                                 prefs::kTokenServiceDiceCompatible));
  }
}

}  // namespace signin
