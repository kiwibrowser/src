// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/placeholder_image.h"

#include <stdint.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_localized_string.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {
namespace {

class TestingUnitsPlatform : public TestingPlatformSupport {
 public:
  TestingUnitsPlatform() {}
  ~TestingUnitsPlatform() override;

  WebString QueryLocalizedString(WebLocalizedString::Name name,
                                 const WebString& parameter) override {
    String p = parameter;
    switch (name) {
      case WebLocalizedString::kUnitsKibibytes:
        return String(p + " KB");
      case WebLocalizedString::kUnitsMebibytes:
        return String(p + " MB");
      case WebLocalizedString::kUnitsGibibytes:
        return String(p + " GB");
      case WebLocalizedString::kUnitsTebibytes:
        return String(p + " TB");
      case WebLocalizedString::kUnitsPebibytes:
        return String(p + " PB");
      default:
        return WebString();
    }
  }
};

TestingUnitsPlatform::~TestingUnitsPlatform() = default;

class PlaceholderImageTest : public testing::Test {
 private:
  ScopedTestingPlatformSupport<TestingUnitsPlatform> platform_;
};

TEST_F(PlaceholderImageTest, FormatPlaceholderText) {
  const struct {
    int64_t bytes;
    const char* expected;
  } tests[] = {
      // Placeholder image number format specifications:
      // https://docs.google.com/document/d/1BHeA1azbgCdZgCnr16VN2g7A9MHPQ_dwKn5szh8evMQ/edit#heading=h.d135l9z7tn0a
      {1, "1 KB"},
      {500, "1 KB"},
      {5 * 1024 + 200, "5 KB"},
      {50 * 1024 + 200, "50 KB"},
      {1000 * 1024 - 1, "999 KB"},
      {1000 * 1024, "1 MB"},
      {1024 * 1024 + 103 * 1024, "1.1 MB"},
      {10 * 1024 * 1024, "10 MB"},
      {10 * 1024 * 1024 + 103 * 1024, "10 MB"},
      {1000 * 1024 * 1024 - 1, "999 MB"},
      {1000 * 1024 * 1024, "1 GB"},
      {1024 * 1024 * 1024, "1 GB"},
      {(1LL << 50), "1 PB"},
      {(1LL << 50) + 103 * (1LL << 40), "1.1 PB"},
      {10 * (1LL << 50), "10 PB"},
      {10 * (1LL << 50) + 103 * (1LL << 40), "10 PB"},
      {~(1LL << 63), "8191 PB"},
  };

  for (const auto& test : tests) {
    String expected = test.expected;
    expected.Ensure16Bit();

    EXPECT_EQ(expected,
              PlaceholderImage::Create(nullptr, IntSize(400, 300), test.bytes)
                  ->GetTextForTesting());
  }
}

}  // namespace

}  // namespace blink
