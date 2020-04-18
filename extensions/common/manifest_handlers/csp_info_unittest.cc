// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/csp_info.h"
#include "extensions/common/manifest_test.h"

namespace extensions {

namespace errors = manifest_errors;

using CSPInfoUnitTest = ManifestTest;

TEST_F(CSPInfoUnitTest, SandboxedPages) {
  // Sandboxed pages specified, no custom CSP value.
  scoped_refptr<Extension> extension1(
      LoadAndExpectSuccess("sandboxed_pages_valid_1.json"));

  // No sandboxed pages.
  scoped_refptr<Extension> extension2(
      LoadAndExpectSuccess("sandboxed_pages_valid_2.json"));

  // Sandboxed pages specified with a custom CSP value.
  scoped_refptr<Extension> extension3(
      LoadAndExpectSuccess("sandboxed_pages_valid_3.json"));

  // Sandboxed pages specified with wildcard, no custom CSP value.
  scoped_refptr<Extension> extension4(
      LoadAndExpectSuccess("sandboxed_pages_valid_4.json"));

  // Sandboxed pages specified with filename wildcard, no custom CSP value.
  scoped_refptr<Extension> extension5(
      LoadAndExpectSuccess("sandboxed_pages_valid_5.json"));

  const char kSandboxedCSP[] =
      "sandbox allow-scripts allow-forms allow-popups allow-modals; "
      "script-src 'self' 'unsafe-inline' 'unsafe-eval'; child-src 'self';";
  const char kDefaultCSP[] =
      "script-src 'self' blob: filesystem: chrome-extension-resource:; "
      "object-src 'self' blob: filesystem:;";
  const char kCustomSandboxedCSP[] =
      "sandbox; script-src 'self'; child-src 'self';";

  EXPECT_EQ(kSandboxedCSP, CSPInfo::GetResourceContentSecurityPolicy(
                               extension1.get(), "/test"));
  EXPECT_EQ(kDefaultCSP, CSPInfo::GetResourceContentSecurityPolicy(
                             extension1.get(), "/none"));
  EXPECT_EQ(kDefaultCSP, CSPInfo::GetResourceContentSecurityPolicy(
                             extension2.get(), "/test"));
  EXPECT_EQ(kCustomSandboxedCSP, CSPInfo::GetResourceContentSecurityPolicy(
                                     extension3.get(), "/test"));
  EXPECT_EQ(kDefaultCSP, CSPInfo::GetResourceContentSecurityPolicy(
                             extension3.get(), "/none"));
  EXPECT_EQ(kSandboxedCSP, CSPInfo::GetResourceContentSecurityPolicy(
                               extension4.get(), "/test"));
  EXPECT_EQ(kSandboxedCSP, CSPInfo::GetResourceContentSecurityPolicy(
                               extension5.get(), "/path/test.ext"));
  EXPECT_EQ(kDefaultCSP, CSPInfo::GetResourceContentSecurityPolicy(
                             extension5.get(), "/test"));

  Testcase testcases[] = {
      Testcase("sandboxed_pages_invalid_1.json",
               errors::kInvalidSandboxedPagesList),
      Testcase("sandboxed_pages_invalid_2.json", errors::kInvalidSandboxedPage),
      Testcase("sandboxed_pages_invalid_3.json",
               errors::kInvalidSandboxedPagesCSP),
      Testcase("sandboxed_pages_invalid_4.json",
               errors::kInvalidSandboxedPagesCSP),
      Testcase("sandboxed_pages_invalid_5.json",
               errors::kInvalidSandboxedPagesCSP)};
  RunTestcases(testcases, arraysize(testcases), EXPECT_TYPE_ERROR);
}

}  // namespace extensions
