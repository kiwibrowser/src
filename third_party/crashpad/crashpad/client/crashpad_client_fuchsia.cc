// Copyright 2017 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "client/crashpad_client.h"

#include <launchpad/launchpad.h>
#include <zircon/process.h>
#include <zircon/processargs.h>

#include "base/fuchsia/fuchsia_logging.h"
#include "base/fuchsia/scoped_zx_handle.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "client/client_argv_handling.h"
#include "util/fuchsia/system_exception_port_key.h"

namespace crashpad {

CrashpadClient::CrashpadClient() {}

CrashpadClient::~CrashpadClient() {}

bool CrashpadClient::StartHandler(
    const base::FilePath& handler,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments,
    bool restartable,
    bool asynchronous_start) {
  DCHECK_EQ(restartable, false);  // Not used on Fuchsia.
  DCHECK_EQ(asynchronous_start, false);  // Not used on Fuchsia.

  zx_handle_t exception_port_raw;
  zx_status_t status = zx_port_create(0, &exception_port_raw);
  if (status != ZX_OK) {
    ZX_LOG(ERROR, status) << "zx_port_create";
    return false;
  }
  base::ScopedZxHandle exception_port(exception_port_raw);

  status = zx_task_bind_exception_port(
      zx_job_default(), exception_port.get(), kSystemExceptionPortKey, 0);
  if (status != ZX_OK) {
    ZX_LOG(ERROR, status) << "zx_task_bind_exception_port";
    return false;
  }

  std::vector<std::string> argv_strings;
  BuildHandlerArgvStrings(handler,
                          database,
                          metrics_dir,
                          url,
                          annotations,
                          arguments,
                          &argv_strings);

  std::vector<const char*> argv;
  ConvertArgvStrings(argv_strings, &argv);
  // ConvertArgvStrings adds an unnecessary nullptr at the end of the argv list,
  // which causes launchpad_set_args() to hang.
  argv.pop_back();

  launchpad_t* lp;
  launchpad_create(zx_job_default(), argv[0], &lp);
  launchpad_load_from_file(lp, argv[0]);
  launchpad_set_args(lp, argv.size(), &argv[0]);

  // TODO(scottmg): https://crashpad.chromium.org/bug/196, this is useful during
  // bringup, but should probably be made minimal for real usage.
  launchpad_clone(lp,
                  LP_CLONE_FDIO_NAMESPACE | LP_CLONE_FDIO_STDIO |
                      LP_CLONE_ENVIRON | LP_CLONE_DEFAULT_JOB);

  // Follow the same protocol as devmgr and crashlogger in Zircon (that is,
  // process handle as handle 0, with type USER0, exception port handle as
  // handle 1, also with type PA_USER0) so that it's trivial to replace
  // crashlogger with crashpad_handler. The exception port is passed on, so
  // released here. Currently it is assumed that this process's default job
  // handle is the exception port that should be monitored. In the future, it
  // might be useful for this to be configurable by the client.
  zx_handle_t handles[] = {ZX_HANDLE_INVALID, ZX_HANDLE_INVALID};
  status =
      zx_handle_duplicate(zx_job_default(), ZX_RIGHT_SAME_RIGHTS, &handles[0]);
  if (status != ZX_OK) {
    ZX_LOG(ERROR, status) << "zx_handle_duplicate";
    return false;
  }
  handles[1] = exception_port.release();
  uint32_t handle_types[] = {PA_HND(PA_USER0, 0), PA_HND(PA_USER0, 1)};

  launchpad_add_handles(lp, arraysize(handles), handles, handle_types);

  const char* error_message;
  zx_handle_t child_raw;
  status = launchpad_go(lp, &child_raw, &error_message);
  base::ScopedZxHandle child(child_raw);
  if (status != ZX_OK) {
    ZX_LOG(ERROR, status) << "launchpad_go: " << error_message;
    return false;
  }

  return true;
}

}  // namespace crashpad
