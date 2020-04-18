// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace {

const char kTestScheme[] = "test-scheme";
const char kTestScheme2[] = "test-scheme-2";

class SchemeRegistryTest : public testing::Test {
  void TearDown() override {
    SchemeRegistry::RemoveURLSchemeRegisteredAsBypassingContentSecurityPolicy(
        kTestScheme);
  }
};

TEST_F(SchemeRegistryTest, NoCSPBypass) {
  EXPECT_FALSE(
      SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(kTestScheme));
  EXPECT_FALSE(SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(
      kTestScheme, SchemeRegistry::kPolicyAreaImage));
}

TEST_F(SchemeRegistryTest, FullCSPBypass) {
  SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
      kTestScheme);
  EXPECT_TRUE(
      SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(kTestScheme));
  EXPECT_TRUE(SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(
      kTestScheme, SchemeRegistry::kPolicyAreaImage));
  EXPECT_FALSE(
      SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(kTestScheme2));
  EXPECT_FALSE(SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(
      kTestScheme2, SchemeRegistry::kPolicyAreaImage));
}

TEST_F(SchemeRegistryTest, PartialCSPBypass) {
  SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
      kTestScheme, SchemeRegistry::kPolicyAreaImage);
  EXPECT_FALSE(
      SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(kTestScheme));
  EXPECT_TRUE(SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(
      kTestScheme, SchemeRegistry::kPolicyAreaImage));
  EXPECT_FALSE(SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(
      kTestScheme, SchemeRegistry::kPolicyAreaStyle));
  EXPECT_FALSE(SchemeRegistry::SchemeShouldBypassContentSecurityPolicy(
      kTestScheme2, SchemeRegistry::kPolicyAreaImage));
}

TEST_F(SchemeRegistryTest, BypassSecureContextCheck) {
  const char* scheme1 = "http";
  const char* scheme2 = "https";
  const char* scheme3 = "random-scheme";

  EXPECT_FALSE(SchemeRegistry::SchemeShouldBypassSecureContextCheck(scheme1));
  EXPECT_FALSE(SchemeRegistry::SchemeShouldBypassSecureContextCheck(scheme2));
  EXPECT_FALSE(SchemeRegistry::SchemeShouldBypassSecureContextCheck(scheme3));

  SchemeRegistry::RegisterURLSchemeBypassingSecureContextCheck("random-scheme");

  EXPECT_FALSE(SchemeRegistry::SchemeShouldBypassSecureContextCheck(scheme1));
  EXPECT_FALSE(SchemeRegistry::SchemeShouldBypassSecureContextCheck(scheme2));
  EXPECT_TRUE(SchemeRegistry::SchemeShouldBypassSecureContextCheck(scheme3));
}

}  // namespace
}  // namespace blink
