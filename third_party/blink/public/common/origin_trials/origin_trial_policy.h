// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_TRIALS_ORIGIN_TRIAL_POLICY_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_TRIALS_ORIGIN_TRIAL_POLICY_H_

#include "base/strings/string_piece.h"
#include "url/gurl.h"

namespace blink {

// The OriginTrialPolicy provides an interface to the TrialTokenValidator used
// to check for disabled features or tokens.
class OriginTrialPolicy {
 public:
  virtual ~OriginTrialPolicy() = default;

  virtual bool IsOriginTrialsSupported() const { return false; }
  virtual base::StringPiece GetPublicKey() const { return base::StringPiece(); }
  virtual bool IsFeatureDisabled(base::StringPiece feature) const {
    return false;
  }
  virtual bool IsTokenDisabled(base::StringPiece token_signature) const {
    return false;
  }
  virtual bool IsOriginSecure(const GURL& url) const { return false; }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_TRIALS_ORIGIN_TRIAL_POLICY_H_
