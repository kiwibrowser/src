// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/origin_trials/chrome_origin_trial_policy.h"

#include <memory>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "chrome/common/chrome_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

const uint8_t kTestPublicKey[] = {
    0x75, 0x10, 0xac, 0xf9, 0x3a, 0x1c, 0xb8, 0xa9, 0x28, 0x70, 0xd2,
    0x9a, 0xd0, 0x0b, 0x59, 0xe1, 0xac, 0x2b, 0xb7, 0xd5, 0xca, 0x1f,
    0x64, 0x90, 0x08, 0x8e, 0xa8, 0xe0, 0x56, 0x3a, 0x04, 0xd0,
};

// Base64 encoding of the above sample public key
const char kTestPublicKeyString[] =
    "dRCs+TocuKkocNKa0AtZ4awrt9XKH2SQCI6o4FY6BNA=";
const char kBadEncodingPublicKeyString[] = "Not even base64!";
// Base64-encoded, 31 bytes long
const char kTooShortPublicKeyString[] =
    "dRCs+TocuKkocNKa0AtZ4awrt9XKH2SQCI6o4FY6BN==";
// Base64-encoded, 33 bytes long
const char kTooLongPublicKeyString[] =
    "dRCs+TocuKkocNKa0AtZ4awrt9XKH2SQCI6o4FY6BNAA";

const char kOneDisabledFeature[] = "A";
const char kTwoDisabledFeatures[] = "A|B";
const char kThreeDisabledFeatures[] = "A|B|C";
const char kSpacesInDisabledFeatures[] = "A|B C";

// Various tokens, each provide the command (in tools/origin_trials) used for
// generation.
// generate_token.py example.com A --expire-timestamp=2000000000
const uint8_t kToken1Signature[] = {
    0x43, 0xdd, 0xd3, 0x2b, 0x12, 0x09, 0x59, 0x52, 0x17, 0xf3, 0x60,
    0x44, 0xab, 0xae, 0x18, 0xcd, 0xcd, 0x20, 0xf4, 0x0f, 0x37, 0x8c,
    0x04, 0x98, 0x8b, 0x8e, 0xf5, 0x7f, 0x56, 0xe3, 0x22, 0xa8, 0xe5,
    0x02, 0x08, 0xfc, 0x2b, 0xd8, 0x6e, 0x91, 0x1f, 0x8f, 0xf1, 0xec,
    0x61, 0xbc, 0x0d, 0xb2, 0x96, 0xcf, 0xc3, 0xf0, 0xc2, 0xc3, 0x23,
    0xe9, 0x34, 0x4f, 0x55, 0x62, 0x46, 0xcb, 0x57, 0x0b};
const char kToken1SignatureEncoded[] =
    "Q93TKxIJWVIX82BEq64Yzc0g9A83jASYi471f1bjIqjlAgj8K9hukR+P8exhvA2yls/"
    "D8MLDI+k0T1ViRstXCw==";
// generate_token.py example.com A --expire-timestamp=2500000000
const uint8_t kToken2Signature[] = {
    0xcd, 0x7f, 0x73, 0xb4, 0x49, 0xf5, 0xff, 0xef, 0xf3, 0x71, 0x4e,
    0x3d, 0xbd, 0x07, 0xcb, 0x94, 0xd7, 0x25, 0x6f, 0x48, 0x14, 0x2f,
    0xb6, 0x9a, 0xc1, 0x33, 0xf6, 0x8f, 0x8f, 0x72, 0xab, 0xd8, 0xeb,
    0x52, 0x5a, 0x20, 0x49, 0xad, 0xf0, 0x84, 0x49, 0x22, 0x64, 0x65,
    0x25, 0xa2, 0xb4, 0xc8, 0x5d, 0xc3, 0xa4, 0x24, 0xaf, 0xac, 0xcd,
    0x48, 0x22, 0xa4, 0x21, 0x1f, 0x2b, 0xf0, 0xb1, 0x02};
const char kToken2SignatureEncoded[] =
    "zX9ztEn1/+/"
    "zcU49vQfLlNclb0gUL7aawTP2j49yq9jrUlogSa3whEkiZGUlorTIXcOkJK+szUgipCEfK/"
    "CxAg==";
// generate_token.py example.com B --expire-timestamp=2000000000
const uint8_t kToken3Signature[] = {
    0x33, 0x49, 0x37, 0x0e, 0x92, 0xbc, 0xf8, 0xf6, 0x71, 0xa9, 0x7a,
    0x46, 0xd5, 0x35, 0x6d, 0x30, 0xd6, 0x89, 0xe3, 0xa4, 0x5b, 0x0b,
    0xae, 0x6c, 0x77, 0x47, 0xe9, 0x5a, 0x20, 0x14, 0x0d, 0x6f, 0xde,
    0xb4, 0x20, 0xe6, 0xce, 0x3a, 0xf1, 0xcb, 0x92, 0xf9, 0xaf, 0xb2,
    0x89, 0x19, 0xce, 0x35, 0xcc, 0x63, 0x5f, 0x59, 0xd9, 0xef, 0x8f,
    0xf9, 0xa1, 0x92, 0xda, 0x8b, 0xda, 0xfd, 0xf1, 0x08};
const char kToken3SignatureEncoded[] =
    "M0k3DpK8+PZxqXpG1TVtMNaJ46RbC65sd0fpWiAUDW/etCDmzjrxy5L5r7KJGc41zGNfWdnvj/"
    "mhktqL2v3xCA==";
const char kTokenSeparator[] = "|";

class ChromeOriginTrialPolicyTest : public testing::Test {
 protected:
  ChromeOriginTrialPolicyTest()
      : token1_signature_(
            std::string(reinterpret_cast<const char*>(kToken1Signature),
                        arraysize(kToken1Signature))),
        token2_signature_(
            std::string(reinterpret_cast<const char*>(kToken2Signature),
                        arraysize(kToken2Signature))),
        token3_signature_(
            std::string(reinterpret_cast<const char*>(kToken3Signature),
                        arraysize(kToken3Signature))),
        two_disabled_tokens_(
            base::JoinString({kToken1SignatureEncoded, kToken2SignatureEncoded},
                             kTokenSeparator)),
        three_disabled_tokens_(
            base::JoinString({kToken1SignatureEncoded, kToken2SignatureEncoded,
                              kToken3SignatureEncoded},
                             kTokenSeparator)),
        manager_(base::WrapUnique(new ChromeOriginTrialPolicy())),
        default_key_(manager_->GetPublicKey().as_string()),
        test_key_(std::string(reinterpret_cast<const char*>(kTestPublicKey),
                              arraysize(kTestPublicKey))) {}

  ChromeOriginTrialPolicy* manager() { return manager_.get(); }
  base::StringPiece default_key() { return default_key_; }
  base::StringPiece test_key() { return test_key_; }
  std::string token1_signature_;
  std::string token2_signature_;
  std::string token3_signature_;
  std::string two_disabled_tokens_;
  std::string three_disabled_tokens_;

 private:
  std::unique_ptr<ChromeOriginTrialPolicy> manager_;
  std::string default_key_;
  std::string test_key_;
};

TEST_F(ChromeOriginTrialPolicyTest, DefaultConstructor) {
  // We don't specify here what the key should be, but make sure that it is
  // returned, is valid, and is consistent.
  base::StringPiece key = manager()->GetPublicKey();
  EXPECT_EQ(32UL, key.size());
  EXPECT_EQ(default_key(), key);
}

TEST_F(ChromeOriginTrialPolicyTest, DefaultKeyIsConsistent) {
  ChromeOriginTrialPolicy manager2;
  EXPECT_EQ(manager()->GetPublicKey(), manager2.GetPublicKey());
}

TEST_F(ChromeOriginTrialPolicyTest, OverridePublicKey) {
  EXPECT_TRUE(manager()->SetPublicKeyFromASCIIString(kTestPublicKeyString));
  EXPECT_NE(default_key(), manager()->GetPublicKey());
  EXPECT_EQ(test_key(), manager()->GetPublicKey());
}

TEST_F(ChromeOriginTrialPolicyTest, OverrideKeyNotBase64) {
  EXPECT_FALSE(
      manager()->SetPublicKeyFromASCIIString(kBadEncodingPublicKeyString));
  EXPECT_EQ(default_key(), manager()->GetPublicKey());
}

TEST_F(ChromeOriginTrialPolicyTest, OverrideKeyTooShort) {
  EXPECT_FALSE(
      manager()->SetPublicKeyFromASCIIString(kTooShortPublicKeyString));
  EXPECT_EQ(default_key(), manager()->GetPublicKey());
}

TEST_F(ChromeOriginTrialPolicyTest, OverrideKeyTooLong) {
  EXPECT_FALSE(manager()->SetPublicKeyFromASCIIString(kTooLongPublicKeyString));
  EXPECT_EQ(default_key(), manager()->GetPublicKey());
}

TEST_F(ChromeOriginTrialPolicyTest, NoDisabledFeatures) {
  EXPECT_FALSE(manager()->IsFeatureDisabled("A"));
  EXPECT_FALSE(manager()->IsFeatureDisabled("B"));
  EXPECT_FALSE(manager()->IsFeatureDisabled("C"));
}

TEST_F(ChromeOriginTrialPolicyTest, DisableOneFeature) {
  EXPECT_TRUE(manager()->SetDisabledFeatures(kOneDisabledFeature));
  EXPECT_TRUE(manager()->IsFeatureDisabled("A"));
  EXPECT_FALSE(manager()->IsFeatureDisabled("B"));
}

TEST_F(ChromeOriginTrialPolicyTest, DisableTwoFeatures) {
  EXPECT_TRUE(manager()->SetDisabledFeatures(kTwoDisabledFeatures));
  EXPECT_TRUE(manager()->IsFeatureDisabled("A"));
  EXPECT_TRUE(manager()->IsFeatureDisabled("B"));
  EXPECT_FALSE(manager()->IsFeatureDisabled("C"));
}

TEST_F(ChromeOriginTrialPolicyTest, DisableThreeFeatures) {
  EXPECT_TRUE(manager()->SetDisabledFeatures(kThreeDisabledFeatures));
  EXPECT_TRUE(manager()->IsFeatureDisabled("A"));
  EXPECT_TRUE(manager()->IsFeatureDisabled("B"));
  EXPECT_TRUE(manager()->IsFeatureDisabled("C"));
}

TEST_F(ChromeOriginTrialPolicyTest, DisableFeatureWithSpace) {
  EXPECT_TRUE(manager()->SetDisabledFeatures(kSpacesInDisabledFeatures));
  EXPECT_TRUE(manager()->IsFeatureDisabled("A"));
  EXPECT_TRUE(manager()->IsFeatureDisabled("B C"));
  EXPECT_FALSE(manager()->IsFeatureDisabled("B"));
  EXPECT_FALSE(manager()->IsFeatureDisabled("C"));
}

TEST_F(ChromeOriginTrialPolicyTest, NoDisabledTokens) {
  EXPECT_FALSE(manager()->IsTokenDisabled(token1_signature_));
  EXPECT_FALSE(manager()->IsTokenDisabled(token2_signature_));
  EXPECT_FALSE(manager()->IsTokenDisabled(token3_signature_));
}

TEST_F(ChromeOriginTrialPolicyTest, DisableOneToken) {
  EXPECT_TRUE(manager()->SetDisabledTokens(kToken1SignatureEncoded));
  EXPECT_TRUE(manager()->IsTokenDisabled(token1_signature_));
  EXPECT_FALSE(manager()->IsTokenDisabled(token2_signature_));
}

TEST_F(ChromeOriginTrialPolicyTest, DisableTwoTokens) {
  EXPECT_TRUE(manager()->SetDisabledTokens(two_disabled_tokens_));
  EXPECT_TRUE(manager()->IsTokenDisabled(token1_signature_));
  EXPECT_TRUE(manager()->IsTokenDisabled(token2_signature_));
  EXPECT_FALSE(manager()->IsTokenDisabled(token3_signature_));
}

TEST_F(ChromeOriginTrialPolicyTest, DisableThreeTokens) {
  EXPECT_TRUE(manager()->SetDisabledTokens(three_disabled_tokens_));
  EXPECT_TRUE(manager()->IsTokenDisabled(token1_signature_));
  EXPECT_TRUE(manager()->IsTokenDisabled(token2_signature_));
  EXPECT_TRUE(manager()->IsTokenDisabled(token3_signature_));
}

// Tests for initialization from command line
class ChromeOriginTrialPolicyInitializationTest
    : public ChromeOriginTrialPolicyTest {
 protected:
  ChromeOriginTrialPolicyInitializationTest() {}

  ChromeOriginTrialPolicy* initialized_manager() {
    return initialized_manager_.get();
  }

  void SetUp() override {
    ChromeOriginTrialPolicyTest::SetUp();

    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    ASSERT_FALSE(command_line->HasSwitch(switches::kOriginTrialPublicKey));
    ASSERT_FALSE(
        command_line->HasSwitch(switches::kOriginTrialDisabledFeatures));
    ASSERT_FALSE(command_line->HasSwitch(switches::kOriginTrialDisabledTokens));

    // Setup command line with various updated values
    // New public key
    command_line->AppendSwitchASCII(switches::kOriginTrialPublicKey,
                                    kTestPublicKeyString);
    // One disabled feature
    command_line->AppendSwitchASCII(switches::kOriginTrialDisabledFeatures,
                                    kOneDisabledFeature);
    // One disabled token
    command_line->AppendSwitchASCII(switches::kOriginTrialDisabledTokens,
                                    kToken1SignatureEncoded);

    initialized_manager_ = base::WrapUnique(new ChromeOriginTrialPolicy());
  }

 private:
  std::unique_ptr<ChromeOriginTrialPolicy> initialized_manager_;
};

TEST_F(ChromeOriginTrialPolicyInitializationTest, PublicKeyInitialized) {
  EXPECT_NE(default_key(), initialized_manager()->GetPublicKey());
  EXPECT_EQ(test_key(), initialized_manager()->GetPublicKey());
}

TEST_F(ChromeOriginTrialPolicyInitializationTest, DisabledFeaturesInitialized) {
  EXPECT_TRUE(initialized_manager()->IsFeatureDisabled("A"));
  EXPECT_FALSE(initialized_manager()->IsFeatureDisabled("B"));
}

TEST_F(ChromeOriginTrialPolicyInitializationTest, DisabledTokensInitialized) {
  EXPECT_TRUE(initialized_manager()->IsTokenDisabled(token1_signature_));
  EXPECT_FALSE(initialized_manager()->IsTokenDisabled(token2_signature_));
}
