// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/logging.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/logging_chrome.h"
#include "content/public/browser/browser_thread.h"

namespace logging {

namespace {

// This should be true for exactly the period between the end of
// InitChromeLogging() and the beginning of CleanupChromeLogging().
bool chrome_logging_redirected_ = false;

void SymlinkSetUp(const base::CommandLine& command_line,
                  const base::FilePath& log_path,
                  const base::FilePath& target_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // ChromeOS always logs through the symlink, so it shouldn't be
  // deleted if it already exists.
  logging::LoggingSettings settings;
  settings.logging_dest = DetermineLoggingDestination(command_line);
  settings.log_file = log_path.value().c_str();
  if (!logging::InitLogging(settings)) {
    DLOG(ERROR) << "Unable to initialize logging to " << log_path.value();
    base::PostTaskWithTraits(
        FROM_HERE, {base::MayBlock()},
        base::Bind(&RemoveSymlinkAndLog, log_path, target_path));
  } else {
    chrome_logging_redirected_ = true;
  }
}

}  // namespace

void RedirectChromeLogging(const base::CommandLine& command_line) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (chrome_logging_redirected_) {
    // TODO: Support multiple active users. http://crbug.com/230345
    LOG(WARNING) << "NOT redirecting logging for multi-profiles case.";
    return;
  }

  if (command_line.HasSwitch(switches::kDisableLoggingRedirect))
    return;

  // Redirect logs to the session log directory, if set.  Otherwise
  // defaults to the profile dir.
  const base::FilePath log_path = GetSessionLogFile(command_line);

  // Always force a new symlink when redirecting.
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&SetUpSymlinkIfNeeded, log_path, true),
      base::BindOnce(&SymlinkSetUp, command_line, log_path));
}

}  // namespace logging
