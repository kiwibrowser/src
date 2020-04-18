// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMPONENTS_BROWSER_WATCHER_EXIT_CODE_WATCHER_WIN_H_
#define COMPONENTS_BROWSER_WATCHER_EXIT_CODE_WATCHER_WIN_H_

#include "base/macros.h"
#include "base/process/process.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "base/win/scoped_handle.h"

namespace browser_watcher {

// Watches for the exit code of a process and records it in a given registry
// location.
class ExitCodeWatcher {
 public:
  // Initialize the watcher with a registry path.
  explicit ExitCodeWatcher(base::StringPiece16 registry_path);
  ~ExitCodeWatcher();

  // Initializes from arguments on |cmd_line|, returns true on success.
  // This function expects |process| to be open with sufficient privilege to
  // wait and retrieve the process exit code.
  // It checks the handle for validity and takes ownership of it.
  // The intent is for this handle to be inherited into the watcher process
  // hosting the instance of this class.
  bool Initialize(base::Process process);

  // Waits for the process to exit and records its exit code in registry.
  // This is a blocking call.
  void WaitForExit();

  const base::Process& process() const { return process_; }
  int exit_code() const { return exit_code_; }

 private:
  // Writes |exit_code| to registry, returns true on success.
  bool WriteProcessExitCode(int exit_code);

  // The registry path the exit codes are written to.
  const base::string16 registry_path_;

  // Watched process and its creation time.
  base::Process process_;
  base::Time process_creation_time_;

  // The exit code of the watched process. Valid after WaitForExit.
  int exit_code_;

  DISALLOW_COPY_AND_ASSIGN(ExitCodeWatcher);
};

}  // namespace browser_watcher

#endif  // COMPONENTS_BROWSER_WATCHER_EXIT_CODE_WATCHER_WIN_H_
