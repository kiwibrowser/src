// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/scoped_account_consistency.h"

#include <map>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/test/scoped_feature_list.h"
#include "components/signin/core/browser/signin_buildflags.h"

namespace signin {

ScopedAccountConsistency::ScopedAccountConsistency(
    AccountConsistencyMethod method) {
  DCHECK_NE(AccountConsistencyMethod::kDisabled, method);
#if !BUILDFLAG(ENABLE_DICE_SUPPORT)
  DCHECK_NE(AccountConsistencyMethod::kDice, method);
  DCHECK_NE(AccountConsistencyMethod::kDiceFixAuthErrors, method);
#endif

#if BUILDFLAG(ENABLE_MIRROR)
  DCHECK_EQ(AccountConsistencyMethod::kMirror, method);
  return;
#endif

  signin::SetGaiaOriginIsolatedCallback(base::Bind([] { return true; }));

  // Set up the account consistency method.
  std::string feature_value;
  switch (method) {
    case AccountConsistencyMethod::kDisabled:
      NOTREACHED();
      break;
    case AccountConsistencyMethod::kMirror:
      feature_value = kAccountConsistencyFeatureMethodMirror;
      break;
    case AccountConsistencyMethod::kDiceFixAuthErrors:
      feature_value = kAccountConsistencyFeatureMethodDiceFixAuthErrors;
      break;
    case AccountConsistencyMethod::kDicePrepareMigration:
      feature_value = kAccountConsistencyFeatureMethodDicePrepareMigration;
      break;
    case AccountConsistencyMethod::kDiceMigration:
      feature_value = kAccountConsistencyFeatureMethodDiceMigration;
      break;
    case AccountConsistencyMethod::kDice:
      feature_value = kAccountConsistencyFeatureMethodDice;
      break;
  }

  std::map<std::string, std::string> feature_params;
  feature_params[kAccountConsistencyFeatureMethodParameter] = feature_value;

  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      kAccountConsistencyFeature, feature_params);
  DCHECK_EQ(method, GetAccountConsistencyMethod());
}

ScopedAccountConsistency::~ScopedAccountConsistency() {}

}  // namespace signin
