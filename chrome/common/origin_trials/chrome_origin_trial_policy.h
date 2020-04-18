// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_ORIGIN_TRIALS_CHROME_ORIGIN_TRIAL_POLICY_H_
#define CHROME_COMMON_ORIGIN_TRIALS_CHROME_ORIGIN_TRIAL_POLICY_H_

#include <set>
#include <string>

#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "third_party/blink/public/common/origin_trials/origin_trial_policy.h"

// This class is instantiated on the main/ui thread, but its methods can be
// accessed from any thread.
class ChromeOriginTrialPolicy : public blink::OriginTrialPolicy {
 public:
  ChromeOriginTrialPolicy();
  ~ChromeOriginTrialPolicy() override;

  // blink::OriginTrialPolicy interface
  bool IsOriginTrialsSupported() const override;
  base::StringPiece GetPublicKey() const override;
  bool IsFeatureDisabled(base::StringPiece feature) const override;
  bool IsTokenDisabled(base::StringPiece token_signature) const override;
  bool IsOriginSecure(const GURL& url) const override;

  bool SetPublicKeyFromASCIIString(const std::string& ascii_public_key);
  bool SetDisabledFeatures(const std::string& disabled_feature_list);
  bool SetDisabledTokens(const std::string& disabled_token_list);

 private:
  std::string public_key_;
  std::set<std::string> disabled_features_;
  std::set<std::string> disabled_tokens_;

  DISALLOW_COPY_AND_ASSIGN(ChromeOriginTrialPolicy);
};

#endif  // CHROME_COMMON_ORIGIN_TRIALS_CHROME_ORIGIN_TRIAL_POLICY_H_
