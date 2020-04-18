// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_WATCHER_STABILITY_REPORT_USER_STREAM_DATA_SOURCE_H_
#define COMPONENTS_BROWSER_WATCHER_STABILITY_REPORT_USER_STREAM_DATA_SOURCE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "third_party/crashpad/crashpad/handler/user_stream_data_source.h"

namespace crashpad {
class MinidumpUserExtensionStreamDataSource;
class ProcessSnapshot;
}  // namespace crashpad

namespace browser_watcher {

// Collects stability instrumentation corresponding to a ProcessSnapshot and
// makes it available to the crash handler.
class StabilityReportUserStreamDataSource
    : public crashpad::UserStreamDataSource {
 public:
  explicit StabilityReportUserStreamDataSource(
      const base::FilePath& user_data_dir);

  std::unique_ptr<crashpad::MinidumpUserExtensionStreamDataSource>
  ProduceStreamData(crashpad::ProcessSnapshot* process_snapshot) override;

 private:
  base::FilePath user_data_dir_;

  DISALLOW_COPY_AND_ASSIGN(StabilityReportUserStreamDataSource);
};

}  // namespace browser_watcher

#endif  // COMPONENTS_BROWSER_WATCHER_STABILITY_REPORT_USER_STREAM_DATA_SOURCE_H_
