// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_WATCHER_STABILITY_PATHS_H_
#define COMPONENTS_BROWSER_WATCHER_STABILITY_PATHS_H_

#include <set>
#include <vector>

#include "base/files/file_path.h"
#include "base/process/process.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <winsock2.h>
#endif  // defined(OS_WIN)

namespace browser_watcher {

#if defined(OS_WIN)

// Returns the stability debugging directory.
base::FilePath GetStabilityDir(const base::FilePath& user_data_dir);

// Returns the stability debugging path, which is based on pid and creation time
// to ensure uniqueness in the face of pid recycling.
base::FilePath GetStabilityFileForProcess(base::ProcessId pid,
                                          timeval creation_time,
                                          const base::FilePath& user_data_dir);

// On success, returns true and |path| contains the path to the stability file.
// On failure, returns false.
bool GetStabilityFileForProcess(const base::Process& process,
                                const base::FilePath& user_data_dir,
                                base::FilePath* path);

// Returns a pattern that matches file names returned by GetFileForProcess.
base::FilePath::StringType GetStabilityFilePattern();

// Returns files in |stability_dir| that match |stability_file_pattern|,
// excluding those in |excluded_stability_files|.
std::vector<base::FilePath> GetStabilityFiles(
    const base::FilePath& stability_dir,
    const base::FilePath::StringType& stability_file_pattern,
    const std::set<base::FilePath>& excluded_stability_files);

// Sets the current process's stability file's state to deleted (via the
// GlobalActivityTracker) and opens the file for deletion. Metrics pertaining to
// stability recording are logged.
void MarkOwnStabilityFileDeleted(const base::FilePath& user_data_dir);

// Sets another process's stability file's state to deleted, then opens it for
// deletion. This function is meant for use by the crashpad handler; it logs
// metrics labelled as in the context of crash collection.
void MarkStabilityFileDeletedOnCrash(const base::FilePath& file_path);

#endif  // defined(OS_WIN)

}  // namespace browser_watcher

#endif  // COMPONENTS_BROWSER_WATCHER_STABILITY_PATHS_H_
