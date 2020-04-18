// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/web_app.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/render_messages.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/test_renderer_host.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(TOOLKIT_VIEWS)
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/favicon/favicon_utils.h"
#endif

class WebApplicationTest : public ChromeRenderViewHostTestHarness {
 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
#if defined(TOOLKIT_VIEWS)
    extensions::TabHelper::CreateForWebContents(web_contents());
    favicon::CreateContentFaviconDriverForWebContents(web_contents());
#endif
  }
};

#if BUILDFLAG(ENABLE_EXTENSIONS)
TEST_F(WebApplicationTest, AppDirWithId) {
  base::FilePath profile_path(FILE_PATH_LITERAL("profile"));
  base::FilePath result(
      web_app::GetWebAppDataDirectory(profile_path, "123", GURL()));
  base::FilePath expected = profile_path.AppendASCII("Web Applications")
                                  .AppendASCII("_crx_123");
  EXPECT_EQ(expected, result);
}

TEST_F(WebApplicationTest, AppDirWithUrl) {
  base::FilePath profile_path(FILE_PATH_LITERAL("profile"));
  base::FilePath result(web_app::GetWebAppDataDirectory(
      profile_path, std::string(), GURL("http://example.com")));
  base::FilePath expected = profile_path.AppendASCII("Web Applications")
      .AppendASCII("example.com").AppendASCII("http_80");
  EXPECT_EQ(expected, result);
}
#endif // ENABLE_EXTENSIONS
