// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using base::ASCIIToUTF16;

namespace {

class OmniboxViewTest : public PlatformTest {
};

TEST_F(OmniboxViewTest, TestStripSchemasUnsafeForPaste) {
  const char* urls[] = {
      " \x01 ",                                       // Safe query.
      "http://www.google.com?q=javascript:alert(0)",  // Safe URL.
      "JavaScript",                                   // Safe query.
      "javaScript:",                                  // Unsafe JS URL.
      " javaScript: ",                                // Unsafe JS URL.
      "javAscript:Javascript:javascript",             // Unsafe JS URL.
      "javAscript:alert(1)",                          // Unsafe JS URL.
      "javAscript:javascript:alert(2)",               // Single strip unsafe.
      "jaVascript:\njavaScript:\x01 alert(3) \x01",   // Single strip unsafe.
      "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16\x17"
      "\x18\x19 JavaScript:alert(4)",  // Leading control chars unsafe.
      "\x01\x02javascript:\x03\x04JavaScript:alert(5)"  // Embedded control
                                                        // characters unsafe.
  };

  const char* expecteds[] = {
      " \x01 ",                                       // Safe query.
      "http://www.google.com?q=javascript:alert(0)",  // Safe URL.
      "JavaScript",                                   // Safe query.
      "",                                             // Unsafe JS URL.
      "",                                             // Unsafe JS URL.
      "javascript",                                   // Unsafe JS URL.
      "alert(1)",                                     // Unsafe JS URL.
      "alert(2)",                                     // Single strip unsafe.
      "alert(3) \x01",                                // Single strip unsafe.
      "alert(4)",  // Leading control chars unsafe.
      "alert(5)"   // Embedded control characters unsafe.
  };

  for (size_t i = 0; i < arraysize(urls); i++) {
    EXPECT_EQ(ASCIIToUTF16(expecteds[i]),
              OmniboxView::StripJavascriptSchemas(base::UTF8ToUTF16(urls[i])));
  }
}

TEST_F(OmniboxViewTest, SanitizeTextForPaste) {
  // Broken URL has newlines stripped.
  const base::string16 kWrappedURL(ASCIIToUTF16(
      "http://www.chromium.org/developers/testing/chromium-\n"
      "build-infrastructure/tour-of-the-chromium-buildbot"));

  const base::string16 kFixedURL(ASCIIToUTF16(
      "http://www.chromium.org/developers/testing/chromium-"
      "build-infrastructure/tour-of-the-chromium-buildbot"));
  EXPECT_EQ(kFixedURL, OmniboxView::SanitizeTextForPaste(kWrappedURL));

  // Multi-line address is converted to a single-line address.
  const base::string16 kWrappedAddress(ASCIIToUTF16(
      "1600 Amphitheatre Parkway\nMountain View, CA"));

  const base::string16 kFixedAddress(ASCIIToUTF16(
      "1600 Amphitheatre Parkway Mountain View, CA"));
  EXPECT_EQ(kFixedAddress, OmniboxView::SanitizeTextForPaste(kWrappedAddress));

  // Line-breaking the JavaScript scheme with no other whitespace results in a
  // dangerous URL that is sanitized by dropping the scheme.
  const base::string16 kDangerousJavaScriptUrl(
      ASCIIToUTF16("java\x0d\x0ascript:alert(0)"));
  const base::string16 kFixedDangerousJavaScriptUrl(ASCIIToUTF16("alert(0)"));
  EXPECT_EQ(kFixedDangerousJavaScriptUrl,
            OmniboxView::SanitizeTextForPaste(kDangerousJavaScriptUrl));

  // Line-breaking the JavaScript scheme with whitespace elsewhere in the string
  // results in a safe string with a space replacing the line break.
  const base::string16 kSafeJavaScriptUrl(
      ASCIIToUTF16("java\x0d\x0ascript: alert(0)"));
  const base::string16 kFixedSafeJavaScriptUrl(
      ASCIIToUTF16("java script: alert(0)"));
  EXPECT_EQ(kFixedSafeJavaScriptUrl,
            OmniboxView::SanitizeTextForPaste(kSafeJavaScriptUrl));
}

}  // namespace
