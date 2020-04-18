// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/origin_trials/trial_token_validator.h"

#include <memory>
#include <set>
#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/origin_trials/origin_trial_policy.h"
#include "third_party/blink/public/common/origin_trials/trial_token.h"
#include "url/gurl.h"

namespace blink {
namespace trial_token_validator_unittest {

// This is a sample public key for testing the API. The corresponding private
// key (use this to generate new samples for this test file) is:
//
//  0x83, 0x67, 0xf4, 0xcd, 0x2a, 0x1f, 0x0e, 0x04, 0x0d, 0x43, 0x13,
//  0x4c, 0x67, 0xc4, 0xf4, 0x28, 0xc9, 0x90, 0x15, 0x02, 0xe2, 0xba,
//  0xfd, 0xbb, 0xfa, 0xbc, 0x92, 0x76, 0x8a, 0x2c, 0x4b, 0xc7, 0x75,
//  0x10, 0xac, 0xf9, 0x3a, 0x1c, 0xb8, 0xa9, 0x28, 0x70, 0xd2, 0x9a,
//  0xd0, 0x0b, 0x59, 0xe1, 0xac, 0x2b, 0xb7, 0xd5, 0xca, 0x1f, 0x64,
//  0x90, 0x08, 0x8e, 0xa8, 0xe0, 0x56, 0x3a, 0x04, 0xd0
const uint8_t kTestPublicKey[] = {
    0x75, 0x10, 0xac, 0xf9, 0x3a, 0x1c, 0xb8, 0xa9, 0x28, 0x70, 0xd2,
    0x9a, 0xd0, 0x0b, 0x59, 0xe1, 0xac, 0x2b, 0xb7, 0xd5, 0xca, 0x1f,
    0x64, 0x90, 0x08, 0x8e, 0xa8, 0xe0, 0x56, 0x3a, 0x04, 0xd0,
};

//  0x21, 0xee, 0xfa, 0x81, 0x6a, 0xff, 0xdf, 0xb8, 0xc1, 0xdd, 0x75,
//  0x05, 0x04, 0x29, 0x68, 0x67, 0x60, 0x85, 0x91, 0xd0, 0x50, 0x16,
//  0x0a, 0xcf, 0xa2, 0x37, 0xa3, 0x2e, 0x11, 0x7a, 0x17, 0x96, 0x50,
//  0x07, 0x4d, 0x76, 0x55, 0x56, 0x42, 0x17, 0x2d, 0x8a, 0x9c, 0x47,
//  0x96, 0x25, 0xda, 0x70, 0xaa, 0xb9, 0xfd, 0x53, 0x5d, 0x51, 0x3e,
//  0x16, 0xab, 0xb4, 0x86, 0xea, 0xf3, 0x35, 0xc6, 0xca
const uint8_t kTestPublicKey2[] = {
    0x50, 0x07, 0x4d, 0x76, 0x55, 0x56, 0x42, 0x17, 0x2d, 0x8a, 0x9c,
    0x47, 0x96, 0x25, 0xda, 0x70, 0xaa, 0xb9, 0xfd, 0x53, 0x5d, 0x51,
    0x3e, 0x16, 0xab, 0xb4, 0x86, 0xea, 0xf3, 0x35, 0xc6, 0xca,
};

// This is a good trial token, signed with the above test private key.
// TODO(iclelland): This token expires in 2033. Update it or find a way
// to autogenerate it before then.
// Generate this token with the command (in tools/origin_trials):
// generate_token.py valid.example.com Frobulate --expire-timestamp=2000000000
const char kSampleToken[] =
    "AuR/1mg+/w5ROLN54Ok20rApK3opgR7Tq9ZfzhATQmnCa+BtPA1RRw4Nigf336r+"
    "O4fM3Sa+MEd+5JcIgSZafw8AAABZeyJvcmlnaW4iOiAiaHR0cHM6Ly92YWxpZC5l"
    "eGFtcGxlLmNvbTo0NDMiLCAiZmVhdHVyZSI6ICJGcm9idWxhdGUiLCAiZXhwaXJ5"
    "IjogMjAwMDAwMDAwMH0=";
const uint8_t kSampleTokenSignature[] = {
    0xe4, 0x7f, 0xd6, 0x68, 0x3e, 0xff, 0x0e, 0x51, 0x38, 0xb3, 0x79,
    0xe0, 0xe9, 0x36, 0xd2, 0xb0, 0x29, 0x2b, 0x7a, 0x29, 0x81, 0x1e,
    0xd3, 0xab, 0xd6, 0x5f, 0xce, 0x10, 0x13, 0x42, 0x69, 0xc2, 0x6b,
    0xe0, 0x6d, 0x3c, 0x0d, 0x51, 0x47, 0x0e, 0x0d, 0x8a, 0x07, 0xf7,
    0xdf, 0xaa, 0xfe, 0x3b, 0x87, 0xcc, 0xdd, 0x26, 0xbe, 0x30, 0x47,
    0x7e, 0xe4, 0x97, 0x08, 0x81, 0x26, 0x5a, 0x7f, 0x0f};

// The token should be valid for this origin and for this feature.
const char kAppropriateOrigin[] = "https://valid.example.com";
const char kAppropriateFeatureName[] = "Frobulate";

const char kInappropriateFeatureName[] = "Grokalyze";
const char kInappropriateOrigin[] = "https://invalid.example.com";
const char kInsecureOrigin[] = "http://valid.example.com";

// Well-formed trial token with an invalid signature.
// This token is a corruption of the above valid token.
const char kInvalidSignatureToken[] =
    "AuR/1mg+/w5ROLN54Ok20rApK3opgR7Tq9ZfzhATQmnCa+BtPA1RRw4Nigf336r+"
    "RrOtlAwa0gPqqn+A8GTD3AQAAABZeyJvcmlnaW4iOiAiaHR0cHM6Ly92YWxpZC5l"
    "eGFtcGxlLmNvbTo0NDMiLCAiZmVhdHVyZSI6ICJGcm9idWxhdGUiLCAiZXhwaXJ5"
    "IjogMjAwMDAwMDAwMH0=";

// Well-formed, but expired, trial token. (Expired in 2001)
// Generate this token with the command (in tools/origin_trials):
// generate_token.py valid.example.com Frobulate --expire-timestamp=1000000000
const char kExpiredToken[] =
    "AmHPUIXMaXe9jWW8kJeDFXolVjT93p4XMnK4+jMYd2pjqtFcYB1bUmdD8PunQKM+"
    "RrOtlAwa0gPqqn+A8GTD3AQAAABZeyJvcmlnaW4iOiAiaHR0cHM6Ly92YWxpZC5l"
    "eGFtcGxlLmNvbTo0NDMiLCAiZmVhdHVyZSI6ICJGcm9idWxhdGUiLCAiZXhwaXJ5"
    "IjogMTAwMDAwMDAwMH0=";
const uint8_t kExpiredTokenSignature[] = {
    0x61, 0xcf, 0x50, 0x85, 0xcc, 0x69, 0x77, 0xbd, 0x8d, 0x65, 0xbc,
    0x90, 0x97, 0x83, 0x15, 0x7a, 0x25, 0x56, 0x34, 0xfd, 0xde, 0x9e,
    0x17, 0x32, 0x72, 0xb8, 0xfa, 0x33, 0x18, 0x77, 0x6a, 0x63, 0xaa,
    0xd1, 0x5c, 0x60, 0x1d, 0x5b, 0x52, 0x67, 0x43, 0xf0, 0xfb, 0xa7,
    0x40, 0xa3, 0x3e, 0x46, 0xb3, 0xad, 0x94, 0x0c, 0x1a, 0xd2, 0x03,
    0xea, 0xaa, 0x7f, 0x80, 0xf0, 0x64, 0xc3, 0xdc, 0x04};

const char kUnparsableToken[] = "abcde";

// Well-formed token, for an insecure origin.
// Generate this token with the command (in tools/origin_trials):
// generate_token.py http://valid.example.com Frobulate
// --expire-timestamp=2000000000
const char kInsecureOriginToken[] =
    "AjfC47H1q8/Ho5ALFkjkwf9CBK6oUUeRTlFc50Dj+eZEyGGKFIY2WTxMBfy8cLc3"
    "E0nmFroDA3OmABmO5jMCFgkAAABXeyJvcmlnaW4iOiAiaHR0cDovL3ZhbGlkLmV4"
    "YW1wbGUuY29tOjgwIiwgImZlYXR1cmUiOiAiRnJvYnVsYXRlIiwgImV4cGlyeSI6"
    "IDIwMDAwMDAwMDB9";

// This timestamp is set to a time after the expiry timestamp of kExpiredToken,
// but before the expiry timestamp of kValidToken.
double kNowTimestamp = 1500000000;

class TestOriginTrialPolicy : public OriginTrialPolicy {
 public:
  bool IsOriginTrialsSupported() const override { return true; }
  bool IsOriginSecure(const GURL& url) const override {
    return url.SchemeIs("https");
  }
  base::StringPiece GetPublicKey() const override {
    return base::StringPiece(reinterpret_cast<const char*>(key_),
                             arraysize(kTestPublicKey));
  }
  bool IsFeatureDisabled(base::StringPiece feature) const override {
    return disabled_features_.count(feature.as_string()) > 0;
  }

  // Test setup methods
  void SetPublicKey(const uint8_t* key) { key_ = key; }
  void DisableFeature(const std::string& feature) {
    disabled_features_.insert(feature);
  }
  void DisableToken(const std::string& token) {
    disabled_tokens_.insert(token);
  }

 protected:
  bool IsTokenDisabled(base::StringPiece token_signature) const override {
    return disabled_tokens_.count(token_signature.as_string()) > 0;
  }

 private:
  const uint8_t* key_ = nullptr;
  std::set<std::string> disabled_features_;
  std::set<std::string> disabled_tokens_;
};

class TrialTokenValidatorTest : public testing::Test {
 public:
  TrialTokenValidatorTest()
      : appropriate_origin_(url::Origin::Create(GURL(kAppropriateOrigin))),
        inappropriate_origin_(url::Origin::Create(GURL(kInappropriateOrigin))),
        insecure_origin_(url::Origin::Create(GURL(kInsecureOrigin))),
        valid_token_signature_(
            std::string(reinterpret_cast<const char*>(kSampleTokenSignature),
                        arraysize(kSampleTokenSignature))),
        expired_token_signature_(
            std::string(reinterpret_cast<const char*>(kExpiredTokenSignature),
                        arraysize(kExpiredTokenSignature))),
        response_headers_(new net::HttpResponseHeaders("")) {
    TrialTokenValidator::SetOriginTrialPolicyGetter(
        base::BindRepeating([](OriginTrialPolicy* policy) { return policy; },
                            base::Unretained(&policy_)));
    SetPublicKey(kTestPublicKey);
  }

  ~TrialTokenValidatorTest() override {
    TrialTokenValidator::ResetOriginTrialPolicyGetter();
  }

  void SetPublicKey(const uint8_t* key) { policy_.SetPublicKey(key); }

  void DisableFeature(const std::string& feature) {
    policy_.DisableFeature(feature);
  }

  void DisableToken(const std::string& token_signature) {
    policy_.DisableToken(token_signature);
  }

  base::Time Now() { return base::Time::FromDoubleT(kNowTimestamp); }

  const url::Origin appropriate_origin_;
  const url::Origin inappropriate_origin_;
  const url::Origin insecure_origin_;

  std::string valid_token_signature_;
  std::string expired_token_signature_;

  scoped_refptr<net::HttpResponseHeaders> response_headers_;

  TestOriginTrialPolicy policy_;
  const TrialTokenValidator validator_;
};

TEST_F(TrialTokenValidatorTest, ValidateValidToken) {
  std::string feature;
  EXPECT_EQ(blink::OriginTrialTokenStatus::kSuccess,
            validator_.ValidateToken(kSampleToken, appropriate_origin_,
                                     &feature, Now()));
  EXPECT_EQ(kAppropriateFeatureName, feature);
}

TEST_F(TrialTokenValidatorTest, ValidateInappropriateOrigin) {
  std::string feature;
  EXPECT_EQ(blink::OriginTrialTokenStatus::kWrongOrigin,
            validator_.ValidateToken(kSampleToken, inappropriate_origin_,
                                     &feature, Now()));
  EXPECT_EQ(blink::OriginTrialTokenStatus::kWrongOrigin,
            validator_.ValidateToken(kSampleToken, insecure_origin_, &feature,
                                     Now()));
}

TEST_F(TrialTokenValidatorTest, ValidateInvalidSignature) {
  std::string feature;
  EXPECT_EQ(blink::OriginTrialTokenStatus::kInvalidSignature,
            validator_.ValidateToken(kInvalidSignatureToken,
                                     appropriate_origin_, &feature, Now()));
}

TEST_F(TrialTokenValidatorTest, ValidateUnparsableToken) {
  std::string feature;
  EXPECT_EQ(blink::OriginTrialTokenStatus::kMalformed,
            validator_.ValidateToken(kUnparsableToken, appropriate_origin_,
                                     &feature, Now()));
}

TEST_F(TrialTokenValidatorTest, ValidateExpiredToken) {
  std::string feature;
  EXPECT_EQ(blink::OriginTrialTokenStatus::kExpired,
            validator_.ValidateToken(kExpiredToken, appropriate_origin_,
                                     &feature, Now()));
}

TEST_F(TrialTokenValidatorTest, ValidateValidTokenWithIncorrectKey) {
  std::string feature;
  SetPublicKey(kTestPublicKey2);
  EXPECT_EQ(blink::OriginTrialTokenStatus::kInvalidSignature,
            validator_.ValidateToken(kSampleToken, appropriate_origin_,
                                     &feature, Now()));
}

TEST_F(TrialTokenValidatorTest, ValidatorRespectsDisabledFeatures) {
  std::string feature;
  // Disable an irrelevant feature; token should still validate
  DisableFeature(kInappropriateFeatureName);
  EXPECT_EQ(blink::OriginTrialTokenStatus::kSuccess,
            validator_.ValidateToken(kSampleToken, appropriate_origin_,
                                     &feature, Now()));
  EXPECT_EQ(kAppropriateFeatureName, feature);
  // Disable the token's feature; it should no longer be valid
  DisableFeature(kAppropriateFeatureName);
  EXPECT_EQ(blink::OriginTrialTokenStatus::kFeatureDisabled,
            validator_.ValidateToken(kSampleToken, appropriate_origin_,
                                     &feature, Now()));
}

TEST_F(TrialTokenValidatorTest, ValidatorRespectsDisabledTokens) {
  std::string feature;
  // Disable an irrelevant token; token should still validate
  DisableToken(expired_token_signature_);
  EXPECT_EQ(blink::OriginTrialTokenStatus::kSuccess,
            validator_.ValidateToken(kSampleToken, appropriate_origin_,
                                     &feature, Now()));
  EXPECT_EQ(kAppropriateFeatureName, feature);
  // Disable the token; it should no longer be valid
  DisableToken(valid_token_signature_);
  EXPECT_EQ(blink::OriginTrialTokenStatus::kTokenDisabled,
            validator_.ValidateToken(kSampleToken, appropriate_origin_,
                                     &feature, Now()));
}

TEST_F(TrialTokenValidatorTest, ValidateRequestInsecure) {
  response_headers_->AddHeader(std::string("Origin-Trial: ") +
                               kInsecureOriginToken);
  EXPECT_FALSE(validator_.RequestEnablesFeature(
      GURL(kInsecureOrigin), response_headers_.get(), kAppropriateFeatureName,
      Now()));
}

TEST_F(TrialTokenValidatorTest, ValidateRequestValidToken) {
  response_headers_->AddHeader(std::string("Origin-Trial: ") + kSampleToken);
  EXPECT_TRUE(validator_.RequestEnablesFeature(GURL(kAppropriateOrigin),
                                               response_headers_.get(),
                                               kAppropriateFeatureName, Now()));
}

TEST_F(TrialTokenValidatorTest, ValidateRequestNoTokens) {
  EXPECT_FALSE(validator_.RequestEnablesFeature(
      GURL(kAppropriateOrigin), response_headers_.get(),
      kAppropriateFeatureName, Now()));
}

TEST_F(TrialTokenValidatorTest, ValidateRequestMultipleHeaders) {
  response_headers_->AddHeader(std::string("Origin-Trial: ") + kSampleToken);
  response_headers_->AddHeader(std::string("Origin-Trial: ") + kExpiredToken);
  EXPECT_TRUE(validator_.RequestEnablesFeature(GURL(kAppropriateOrigin),
                                               response_headers_.get(),
                                               kAppropriateFeatureName, Now()));
  EXPECT_FALSE(validator_.RequestEnablesFeature(
      GURL(kAppropriateOrigin), response_headers_.get(),
      kInappropriateFeatureName, Now()));
  EXPECT_FALSE(validator_.RequestEnablesFeature(
      GURL(kInappropriateOrigin), response_headers_.get(),
      kAppropriateFeatureName, Now()));
}

TEST_F(TrialTokenValidatorTest, ValidateRequestMultipleHeaderValues) {
  response_headers_->AddHeader(std::string("Origin-Trial: ") + kExpiredToken +
                               ", " + kSampleToken);
  EXPECT_TRUE(validator_.RequestEnablesFeature(GURL(kAppropriateOrigin),
                                               response_headers_.get(),
                                               kAppropriateFeatureName, Now()));
  EXPECT_FALSE(validator_.RequestEnablesFeature(
      GURL(kAppropriateOrigin), response_headers_.get(),
      kInappropriateFeatureName, Now()));
  EXPECT_FALSE(validator_.RequestEnablesFeature(
      GURL(kInappropriateOrigin), response_headers_.get(),
      kAppropriateFeatureName, Now()));
}

}  // namespace trial_token_validator_unittest
}  // namespace blink
