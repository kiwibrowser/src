// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_BROWSER_INFO_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_BROWSER_INFO_H_

#include "chrome/test/chromedriver/chrome/status.h"

// Content Shell and WebView have an empty product version and a fake user
// agent. There's no way to detect the actual version, so unless specified we
// assume it is tip of tree.
static const int kToTBuildNo = 9999;

// Similarly, if the Blink Revision isn't given then assume it is tip of tree.
static const int kToTBlinkRevision = 999999;

struct BrowserInfo {
  BrowserInfo();
  BrowserInfo(std::string android_package_,
              std::string browser_name_,
              std::string browser_version_,
              int major_version_,
              int build_no_,
              int blink_revision_,
              bool is_android_);
  ~BrowserInfo();

  std::string android_package;
  std::string browser_name;
  std::string browser_version;
  std::string web_socket_url;
  int major_version;
  int build_no;
  int blink_revision;
  bool is_android;
};

Status ParseBrowserInfo(const std::string& data,
                        BrowserInfo* browser_info);

Status ParseBrowserString(bool has_android_package,
                          const std::string& browser_string,
                          BrowserInfo* browser_info);

Status ParseBrowserVersionString(const std::string& browser_version,
                                 int* major_version, int* build_no);

Status ParseBlinkVersionString(const std::string& blink_version,
                               int* blink_revision);

bool IsGitHash(const std::string& revision);

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_BROWSER_INFO_H_
