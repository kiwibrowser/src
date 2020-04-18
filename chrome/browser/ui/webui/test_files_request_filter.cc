// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/test_files_request_filter.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/common/chrome_paths.h"

namespace {

bool HandleTestFileRequestCallback(
    const std::string& path,
    const content::WebUIDataSource::GotDataCallback& callback) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  std::vector<std::string> url_substr =
      base::SplitString(path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (url_substr.size() != 2 || url_substr[0] != "test")
    return false;

  std::string contents;
  base::FilePath test_data_dir;
  base::PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir);
  if (!base::ReadFileToString(
          test_data_dir.AppendASCII("webui").AppendASCII(url_substr[1]),
          &contents))
    return false;

  base::RefCountedString* ref_contents = new base::RefCountedString();
  ref_contents->data() = contents;
  callback.Run(ref_contents);
  return true;
}

}  // namespace

namespace test {

content::WebUIDataSource::HandleRequestCallback GetTestFilesRequestFilter() {
  return base::Bind(&HandleTestFileRequestCallback);
}

}  // namespace test
