// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/common/shell_switches.h"

#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "content/shell/common/layout_test/layout_test_switches.h"

namespace switches {

// Tells Content Shell that it's running as a content_browsertest.
const char kContentBrowserTest[] = "browser-test";

// Makes Content Shell use the given path for its data directory.
const char kContentShellDataPath[] = "data-path";

// The directory breakpad should store minidumps in.
const char kCrashDumpsDir[] = "crash-dumps-dir";

// Exposes the window.internals object to JavaScript for interactive development
// and debugging of layout tests that rely on it.
const char kExposeInternalsForTesting[] = "expose-internals-for-testing";

// Enable site isolation (--site-per-process style isolation) for a subset of
// sites. The argument is a wildcard pattern which will be matched against the
// site URL to determine which sites to isolate. This can be used to isolate
// just one top-level domain, or just one scheme. Example usages:
//     --isolate-sites-for-testing=*.com
//     --isolate-sites-for-testing=https://*
const char kIsolateSitesForTesting[] = "isolate-sites-for-testing";

// Registers additional font files on Windows (for fonts outside the usual
// %WINDIR%\Fonts location). Multiple files can be used by separating them
// with a semicolon (;).
const char kRegisterFontFiles[] = "register-font-files";

// Size for the content_shell's host window (i.e. "800x600").
const char kContentShellHostWindowSize[] = "content-shell-host-window-size";

// Hides toolbar from content_shell's host window.
const char kContentShellHideToolbar[] = "content-shell-hide-toolbar";

// Forces all navigations to go through the browser process (in a
// non-PlzNavigate way).
const char kContentShellAlwaysFork[] = "content-shell-always-fork";

std::vector<std::string> GetSideloadFontFiles() {
  std::vector<std::string> files;
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kRegisterFontFiles)) {
    files = base::SplitString(
        command_line.GetSwitchValueASCII(switches::kRegisterFontFiles),
        ";", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  }
  return files;
}

bool IsRunWebTestsSwitchPresent() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kRunWebTests);
}

}  // namespace switches
