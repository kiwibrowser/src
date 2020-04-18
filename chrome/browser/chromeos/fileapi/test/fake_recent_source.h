// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILEAPI_TEST_FAKE_RECENT_SOURCE_H_
#define CHROME_BROWSER_CHROMEOS_FILEAPI_TEST_FAKE_RECENT_SOURCE_H_

#include <vector>

#include "base/macros.h"
#include "chrome/browser/chromeos/fileapi/recent_source.h"

namespace chromeos {

class RecentFile;

// Fake implementation of RecentSource that returns a canned set of files.
//
// All member functions must be called on the UI thread.
class FakeRecentSource : public RecentSource {
 public:
  FakeRecentSource();
  ~FakeRecentSource() override;

  // Add a file to the canned set.
  void AddFile(const RecentFile& file);

  // RecentSource overrides:
  void GetRecentFiles(Params params) override;

 private:
  std::vector<RecentFile> canned_files_;

  DISALLOW_COPY_AND_ASSIGN(FakeRecentSource);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_TEST_FAKE_RECENT_SOURCE_H_
