// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_WATCHER_POSTMORTEM_MINIDUMP_WRITER_H_
#define COMPONENTS_BROWSER_WATCHER_POSTMORTEM_MINIDUMP_WRITER_H_

#include <stdint.h>

#include <string>

#include "components/browser_watcher/stability_report.pb.h"
#include "third_party/crashpad/crashpad/util/file/file_writer.h"
#include "third_party/crashpad/crashpad/util/misc/uuid.h"

namespace browser_watcher {

// Write to |minidump_file| a minimal minidump that wraps |report|. Returns
// true on success, false otherwise.
// Note: the caller owns |minidump_file| and is responsible for keeping it valid
// for this function's duration. |minidump_file| is expected to be empty
// and a binary stream.
bool WritePostmortemDump(crashpad::FileWriterInterface* minidump_file,
                         const crashpad::UUID& client_id,
                         const crashpad::UUID& report_id,
                         StabilityReport* report);

}  // namespace browser_watcher

#endif  // COMPONENTS_BROWSER_WATCHER_POSTMORTEM_MINIDUMP_WRITER_H_
