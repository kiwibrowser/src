// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/origin_trials/trial_token_validator.h"

#include <memory>
#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/time/time.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "third_party/blink/public/common/origin_trials/origin_trial_policy.h"
#include "third_party/blink/public/common/origin_trials/trial_token.h"

namespace {

static base::RepeatingCallback<blink::OriginTrialPolicy*()>& PolicyGetter() {
  static base::NoDestructor<
      base::RepeatingCallback<blink::OriginTrialPolicy*()>>
      policy(base::BindRepeating(
          []() -> blink::OriginTrialPolicy* { return nullptr; }));
  return *policy;
}
}

namespace blink {

TrialTokenValidator::TrialTokenValidator() {}

TrialTokenValidator::~TrialTokenValidator() = default;

void TrialTokenValidator::SetOriginTrialPolicyGetter(
    base::RepeatingCallback<OriginTrialPolicy*()> policy_getter) {
  PolicyGetter() = policy_getter;
}

void TrialTokenValidator::ResetOriginTrialPolicyGetter() {
  SetOriginTrialPolicyGetter(base::BindRepeating(
      []() -> blink::OriginTrialPolicy* { return nullptr; }));
}

OriginTrialPolicy* TrialTokenValidator::Policy() {
  return PolicyGetter().Run();
}

OriginTrialTokenStatus TrialTokenValidator::ValidateToken(
    base::StringPiece token,
    const url::Origin& origin,
    std::string* feature_name,
    base::Time current_time) const {
  OriginTrialPolicy* policy = Policy();

  if (!policy->IsOriginTrialsSupported())
    return OriginTrialTokenStatus::kNotSupported;

  // TODO(iclelland): Allow for multiple signing keys, and iterate over all
  // active keys here. https://crbug.com/543220
  base::StringPiece public_key = policy->GetPublicKey();

  OriginTrialTokenStatus status;
  std::unique_ptr<TrialToken> trial_token =
      TrialToken::From(token, public_key, &status);
  if (status != OriginTrialTokenStatus::kSuccess)
    return status;

  status = trial_token->IsValid(origin, current_time);
  if (status != OriginTrialTokenStatus::kSuccess)
    return status;

  if (policy->IsFeatureDisabled(trial_token->feature_name()))
    return OriginTrialTokenStatus::kFeatureDisabled;

  if (policy->IsTokenDisabled(trial_token->signature()))
    return OriginTrialTokenStatus::kTokenDisabled;

  *feature_name = trial_token->feature_name();
  return OriginTrialTokenStatus::kSuccess;
}

bool TrialTokenValidator::RequestEnablesFeature(const net::URLRequest* request,
                                                base::StringPiece feature_name,
                                                base::Time current_time) const {
  // TODO(mek): Possibly cache the features that are availble for request in
  // UserData associated with the request.
  return RequestEnablesFeature(request->url(), request->response_headers(),
                               feature_name, current_time);
}

bool TrialTokenValidator::RequestEnablesFeature(
    const GURL& request_url,
    const net::HttpResponseHeaders* response_headers,
    base::StringPiece feature_name,
    base::Time current_time) const {
  if (!IsTrialPossibleOnOrigin(request_url))
    return false;

  url::Origin origin = url::Origin::Create(request_url);
  size_t iter = 0;
  std::string token;
  while (response_headers->EnumerateHeader(&iter, "Origin-Trial", &token)) {
    std::string token_feature;
    // TODO(mek): Log the validation errors to histograms?
    if (ValidateToken(token, origin, &token_feature, current_time) ==
        OriginTrialTokenStatus::kSuccess)
      if (token_feature == feature_name)
        return true;
  }
  return false;
}

std::unique_ptr<TrialTokenValidator::FeatureToTokensMap>
TrialTokenValidator::GetValidTokensFromHeaders(
    const url::Origin& origin,
    const net::HttpResponseHeaders* headers,
    base::Time current_time) const {
  std::unique_ptr<FeatureToTokensMap> tokens(
      std::make_unique<FeatureToTokensMap>());
  if (!IsTrialPossibleOnOrigin(origin))
    return tokens;

  size_t iter = 0;
  std::string token;
  while (headers->EnumerateHeader(&iter, "Origin-Trial", &token)) {
    std::string token_feature;
    if (TrialTokenValidator::ValidateToken(token, origin, &token_feature,
                                           current_time) ==
        OriginTrialTokenStatus::kSuccess) {
      (*tokens)[token_feature].push_back(token);
    }
  }
  return tokens;
}

std::unique_ptr<TrialTokenValidator::FeatureToTokensMap>
TrialTokenValidator::GetValidTokens(const url::Origin& origin,
                                    const FeatureToTokensMap& tokens,
                                    base::Time current_time) const {
  std::unique_ptr<FeatureToTokensMap> out_tokens(
      std::make_unique<FeatureToTokensMap>());
  if (!IsTrialPossibleOnOrigin(origin))
    return out_tokens;

  for (const auto& feature : tokens) {
    for (const std::string& token : feature.second) {
      std::string token_feature;
      if (TrialTokenValidator::ValidateToken(token, origin, &token_feature,
                                             current_time) ==
          OriginTrialTokenStatus::kSuccess) {
        DCHECK_EQ(token_feature, feature.first);
        (*out_tokens)[feature.first].push_back(token);
      }
    }
  }
  return out_tokens;
}

bool TrialTokenValidator::IsTrialPossibleOnOrigin(const GURL& url) const {
  OriginTrialPolicy* policy = Policy();
  return policy && policy->IsOriginTrialsSupported() &&
         policy->IsOriginSecure(url);
}

bool TrialTokenValidator::IsTrialPossibleOnOrigin(
    const url::Origin& origin) const {
  return IsTrialPossibleOnOrigin(origin.GetURL());
}

}  // namespace blink
