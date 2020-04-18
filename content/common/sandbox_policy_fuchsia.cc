// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/sandbox_policy_fuchsia.h"

#include <launchpad/launchpad.h>
#include <zircon/processargs.h>

#include "base/base_paths_fuchsia.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "content/public/common/content_switches.h"
#include "services/service_manager/sandbox/switches.h"

namespace content {

void UpdateLaunchOptionsForSandbox(service_manager::SandboxType type,
                                   base::LaunchOptions* options) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          service_manager::switches::kNoSandbox)) {
    type = service_manager::SANDBOX_TYPE_NO_SANDBOX;
  }

  if (type != service_manager::SANDBOX_TYPE_NO_SANDBOX) {
    // Map /pkg (read-only files deployed from the package) and /tmp into the
    // child's namespace.
    options->paths_to_map.push_back(base::GetPackageRoot());
    base::FilePath temp_dir;
    base::GetTempDir(&temp_dir);
    options->paths_to_map.push_back(temp_dir);

    // Clear environmental variables to better isolate the child from
    // this process.
    options->clear_environ = true;

    // Propagate stdout/stderr/stdin to the child.
    options->clone_flags = LP_CLONE_FDIO_STDIO;
    return;
  }

  DCHECK_EQ(type, service_manager::SANDBOX_TYPE_NO_SANDBOX);
  options->clone_flags =
      LP_CLONE_FDIO_NAMESPACE | LP_CLONE_DEFAULT_JOB | LP_CLONE_FDIO_STDIO;
  options->clear_environ = false;
}

}  // namespace content
