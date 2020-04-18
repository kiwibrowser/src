// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/platform_util.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/version.h"
#include "chrome/browser/platform_util_internal.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace platform_util {

namespace {

const char kNautilusKey[] = "nautilus.desktop";
const char kNautilusKeyExtended[] = "nautilus-folder-handler.desktop";
const char kNautilusCmd[] = "nautilus";
const char kSupportedNautilusVersion[] = "3.0.2";

void RunCommand(const std::string& command,
                const base::FilePath& working_directory,
                const std::string& arg) {
  std::vector<std::string> argv;
  argv.push_back(command);
  argv.push_back(arg);

  base::LaunchOptions options;
  options.current_directory = working_directory;
  options.allow_new_privs = true;
  // xdg-open can fall back on mailcap which eventually might plumb through
  // to a command that needs a terminal.  Set the environment variable telling
  // it that we definitely don't have a terminal available and that it should
  // bring up a new terminal if necessary.  See "man mailcap".
  options.environ["MM_NOTTTY"] = "1";

  // In Google Chrome, we do not let GNOME's bug-buddy intercept our crashes.
  // However, we do not want this environment variable to propagate to external
  // applications. See http://crbug.com/24120
  char* disable_gnome_bug_buddy = getenv("GNOME_DISABLE_CRASH_DIALOG");
  if (disable_gnome_bug_buddy &&
      disable_gnome_bug_buddy == std::string("SET_BY_GOOGLE_CHROME"))
    options.environ["GNOME_DISABLE_CRASH_DIALOG"] = std::string();

  base::Process process = base::LaunchProcess(argv, options);
  if (process.IsValid())
    base::EnsureProcessGetsReaped(std::move(process));
}

void XDGOpen(const base::FilePath& working_directory, const std::string& path) {
  RunCommand("xdg-open", working_directory, path);
}

void XDGEmail(const std::string& email) {
  RunCommand("xdg-email", base::FilePath(), email);
}

void ShowFileInNautilus(const base::FilePath& working_directory,
                        const std::string& path) {
  RunCommand(kNautilusCmd, working_directory, path);
}

std::string GetNautilusVersion() {
  std::string output;
  std::string found_version;

  base::CommandLine nautilus_cl((base::FilePath(kNautilusCmd)));
  nautilus_cl.AppendArg("--version");

  if (base::GetAppOutputAndError(nautilus_cl, &output)) {
    // It is assumed that "nautilus --version" returns something like
    // "GNOME nautilus 3.14.2". First, find the position of the first char of
    // "nautilus " and skip the whole string to get the position of
    // version in the |output| string.
    size_t nautilus_position = output.find("nautilus ");
    size_t version_position = nautilus_position + strlen("nautilus ");
    if (nautilus_position != std::string::npos) {
      found_version = output.substr(version_position);
      base::TrimWhitespaceASCII(found_version,
                                base::TRIM_TRAILING,
                                &found_version);
    }
  }
  return found_version;
}

bool CheckNautilusIsDefault() {
  std::string file_browser;

  base::CommandLine xdg_mime(base::FilePath("xdg-mime"));
  xdg_mime.AppendArg("query");
  xdg_mime.AppendArg("default");
  xdg_mime.AppendArg("inode/directory");

  bool success = base::GetAppOutputAndError(xdg_mime, &file_browser);
  base::TrimWhitespaceASCII(file_browser,
                            base::TRIM_TRAILING,
                            &file_browser);

  if (!success ||
      (file_browser != kNautilusKey && file_browser != kNautilusKeyExtended))
    return false;

  const base::Version supported_version(kSupportedNautilusVersion);
  DCHECK(supported_version.IsValid());
  const base::Version current_version(GetNautilusVersion());
  return current_version.IsValid() && current_version >= supported_version;
}

void ShowItem(Profile* profile,
              const base::FilePath& full_path,
              bool use_nautilus_file_browser) {
  if (use_nautilus_file_browser) {
    OpenItem(profile, full_path, SHOW_ITEM_IN_FOLDER, OpenOperationCallback());
  } else {
    // TODO(estade): It would be nice to be able to select the file in other
    // file managers, but that probably requires extending xdg-open.
    // For now just show the folder for non-Nautilus users.
    OpenItem(profile, full_path.DirName(), OPEN_FOLDER,
             OpenOperationCallback());
  }
}

}  // namespace

namespace internal {

void PlatformOpenVerifiedItem(const base::FilePath& path, OpenItemType type) {
  switch (type) {
    case OPEN_FILE:
      XDGOpen(path.DirName(), path.value());
      break;
    case OPEN_FOLDER:
      // The utility process checks the working directory prior to the
      // invocation of xdg-open by changing the current directory into it. This
      // operation only succeeds if |path| is a directory. Opening "." from
      // there ensures that the target of the operation is a directory.  Note
      // that there remains a TOCTOU race where the directory could be unlinked
      // between the time the utility process changes into the directory and the
      // time the application invoked by xdg-open inspects the path by name.
      XDGOpen(path, ".");
      break;
    case SHOW_ITEM_IN_FOLDER:
      ShowFileInNautilus(path.DirName(), path.value());
      break;
  }
}

}  // namespace internal

void ShowItemInFolder(Profile* profile, const base::FilePath& full_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::WithBaseSyncPrimitives(), base::MayBlock(),
       base::TaskPriority::USER_BLOCKING},
      base::BindOnce(&CheckNautilusIsDefault),
      base::BindOnce(&ShowItem, profile, full_path));
}

void OpenExternal(Profile* profile, const GURL& url) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (url.SchemeIs("mailto"))
    XDGEmail(url.spec());
  else
    XDGOpen(base::FilePath(), url.spec());
}

}  // namespace platform_util
