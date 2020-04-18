// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/app/paths_mac.h"

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/path_service.h"
#include "content/public/common/content_paths.h"

namespace extensions {

namespace {

base::FilePath GetFrameworksPath() {
  base::FilePath path;
  base::PathService::Get(base::FILE_EXE, &path);
  // We now have a path .../App Shell.app/Contents/MacOS/App Shell, and want to
  // transform it into
  // .../App Shell.app/Contents/Frameworks/App Shell Framework.framework.
  // But if it's App Shell Helper.app (inside Frameworks), we must go deeper.
  if (base::mac::IsBackgroundOnlyProcess()) {
    path = path.DirName().DirName().DirName().DirName().DirName();
  } else {
    path = path.DirName().DirName();
  }
  DCHECK_EQ("Contents", path.BaseName().value());
  path = path.Append("Frameworks");
  return path;
}

}  // namespace

void OverrideFrameworkBundlePath() {
  base::FilePath path =
      GetFrameworksPath().Append("App Shell Framework.framework");
  base::mac::SetOverrideFrameworkBundlePath(path);
}

void OverrideChildProcessFilePath() {
  base::FilePath path = GetFrameworksPath()
                            .Append("App Shell Helper.app")
                            .Append("Contents")
                            .Append("MacOS")
                            .Append("App Shell Helper");
  base::PathService::Override(content::CHILD_PROCESS_EXE, path);
}

}  // namespace extensions
