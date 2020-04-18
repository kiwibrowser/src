// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_TEST_INFO_EXTRACTOR_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_TEST_INFO_EXTRACTOR_H_

#include <stddef.h>

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "url/gurl.h"

namespace content {

struct TestInfo {
  TestInfo(const GURL& url,
           bool enable_pixel_dumping,
           const std::string& expected_pixel_hash,
           const base::FilePath& current_working_directory);
  ~TestInfo();

  GURL url;
  bool enable_pixel_dumping;
  std::string expected_pixel_hash;
  base::FilePath current_working_directory;
};

class TestInfoExtractor {
 public:
  explicit TestInfoExtractor(const base::CommandLine& cmd_line);
  ~TestInfoExtractor();

  std::unique_ptr<TestInfo> GetNextTest();

 private:
  base::CommandLine::StringVector cmdline_args_;
  size_t cmdline_position_;

  DISALLOW_COPY_AND_ASSIGN(TestInfoExtractor);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_TEST_INFO_EXTRACTOR_H_
