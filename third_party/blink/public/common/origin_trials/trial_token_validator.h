// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_TRIALS_TRIAL_TOKEN_VALIDATOR_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_TRIALS_TRIAL_TOKEN_VALIDATOR_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "base/callback.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "third_party/blink/common/common_export.h"
#include "url/origin.h"

namespace net {
class HttpResponseHeaders;
class URLRequest;
}  // namespace net

namespace blink {

class OriginTrialPolicy;
enum class OriginTrialTokenStatus;

// TrialTokenValidator checks that a page's OriginTrial token enables a certain
// feature.
//
// Given a policy, feature and token, this class determines if the feature
// should be enabled or not for a specific document.
class BLINK_COMMON_EXPORT TrialTokenValidator {
 public:
  TrialTokenValidator();
  virtual ~TrialTokenValidator();

  using FeatureToTokensMap = std::map<std::string /* feature_name */,
                                      std::vector<std::string /* token */>>;

  // If token validates, |*feature_name| is set to the name of the feature the
  // token enables.
  // This method is thread-safe.
  virtual OriginTrialTokenStatus ValidateToken(base::StringPiece token,
                                               const url::Origin& origin,
                                               std::string* feature_name,
                                               base::Time current_time) const;

  bool RequestEnablesFeature(const net::URLRequest* request,
                             base::StringPiece feature_name,
                             base::Time current_time) const;

  bool RequestEnablesFeature(const GURL& request_url,
                             const net::HttpResponseHeaders* response_headers,
                             base::StringPiece feature_name,
                             base::Time current_time) const;

  // Returns all valid tokens in |headers|.
  std::unique_ptr<FeatureToTokensMap> GetValidTokensFromHeaders(
      const url::Origin& origin,
      const net::HttpResponseHeaders* headers,
      base::Time current_time) const;

  // Returns all valid tokens in |tokens|. This method is used to re-validate
  // previously stored tokens.
  std::unique_ptr<FeatureToTokensMap> GetValidTokens(
      const url::Origin& origin,
      const FeatureToTokensMap& tokens,
      base::Time current_time) const;

  static void SetOriginTrialPolicyGetter(
      base::RepeatingCallback<OriginTrialPolicy*()> policy);
  static void ResetOriginTrialPolicyGetter();
  static OriginTrialPolicy* Policy();

 private:
  bool IsTrialPossibleOnOrigin(const url::Origin& origin) const;
  bool IsTrialPossibleOnOrigin(const GURL& url) const;
};  // class TrialTokenValidator

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_TRIALS_TRIAL_TOKEN_VALIDATOR_H_
