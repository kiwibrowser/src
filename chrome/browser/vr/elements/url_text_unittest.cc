// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/url_text.h"

#include "base/bind.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

#if !defined(OS_LINUX) && !defined(OS_WIN)
// TODO(crbug/731894): This test does not work on Linux.
// TODO(crbug/770893): This test does not work on Windows.
TEST(UrlText, WillNotFailOnNonAsciiURLs) {
  bool unhandled_code_point = false;
  auto url_text = std::make_unique<UrlText>(
      0.010, base::BindRepeating([](bool* flag) { *flag = true; },
                                 base::Unretained(&unhandled_code_point)));
  url_text->SetFieldWidth(1);
  url_text->SetUrl(GURL("http://中央大学.ಠ_ಠ.tw/"));
  url_text->PrepareToDrawForTest();
  EXPECT_EQ(false, unhandled_code_point);
}
#endif

TEST(UrlText, WillFailOnUnhandledCodePoint) {
  bool unhandled_code_point;
  auto url_text = std::make_unique<UrlText>(
      0.010, base::BindRepeating([](bool* flag) { *flag = true; },
                                 base::Unretained(&unhandled_code_point)));
  url_text->SetFieldWidth(1);

  unhandled_code_point = false;
  url_text->SetUrl(GURL("https://foo.com"));
  url_text->PrepareToDrawForTest();
  EXPECT_EQ(false, unhandled_code_point);

  unhandled_code_point = false;
  url_text->SetUnsupportedCodePointsForTest(true);
  url_text->SetUrl(GURL("https://bar.com"));
  url_text->PrepareToDrawForTest();
  EXPECT_EQ(true, unhandled_code_point);

  unhandled_code_point = false;
  url_text->SetUnsupportedCodePointsForTest(false);
  url_text->SetUrl(GURL("https://baz.com"));
  url_text->PrepareToDrawForTest();
  EXPECT_EQ(false, unhandled_code_point);
}

}  // namespace vr
