// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The entry point for all Mac Chromium processes, including the outer app
// bundle (browser) and helper app (renderer, plugin, and friends).

#include <dlfcn.h>
#include <errno.h>
#include <libgen.h>
#include <mach-o/dyld.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "chrome/common/chrome_version.h"

#if defined(HELPER_EXECUTABLE)
#include "sandbox/mac/seatbelt_exec.h"
#endif  // defined(HELPER_EXECUTABLE)

namespace {

typedef int (*ChromeMainPtr)(int, char**);

#if defined(HELPER_EXECUTABLE)
// The command line parameter to engage the v2 sandbox.
constexpr char v2_sandbox_arg[] = "--v2-sandbox";
// The command line paramter indicating that the v2 sandbox is enabled. This
// must be different than the "v2-sandbox" flag to avoid endless re-executing.
// The flag tells the sandbox initialization code inside Chrome that the sandbox
// should already be enabled.
// TODO(kerrnel): Remove this once the V2 sandbox migration is complete.
constexpr char v2_sandbox_enabled_arg[] = "--v2-sandbox-enabled";
// The command line parameter for the file descriptor used to receive the
// sandbox policy.
constexpr char fd_mapping_arg[] = "--fd_mapping=";

void SandboxInit(const char* exec_path,
                 int argc,
                 char* argv[],
                 int fd_mapping) {
  char rp[MAXPATHLEN];
  if (realpath(exec_path, rp) == NULL) {
    perror("realpath");
    abort();
  }

  sandbox::SeatbeltExecServer server(fd_mapping);

  // The name of the parameter containing the executable path.
  const std::string exec_param = "EXECUTABLE_PATH";
  // The name of the parameter containg the pid of this process.
  const std::string pid_param = "CURRENT_PID";

  if (!server.SetParameter(exec_param, rp) ||
      !server.SetParameter(pid_param, std::to_string(getpid()))) {
    fprintf(stderr, "Failed to set up parameters for sandbox.\n");
    abort();
  }

  if (!server.InitializeSandbox()) {
    fprintf(stderr, "Failed to initialize sandbox.\n");
    abort();
  }

  std::vector<char*> new_argv;
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], v2_sandbox_arg) != 0 &&
        strncmp(argv[i], fd_mapping_arg, strlen(fd_mapping_arg)) != 0) {
      new_argv.push_back(argv[i]);
    }
  }
  // Tell Chrome that the sandbox should already be enabled.
  // Note that execv() is documented to treat the argv as constants, so the
  // const_cast is safe.
  new_argv.push_back(const_cast<char*>(v2_sandbox_enabled_arg));
  new_argv.push_back(nullptr);
}
#endif  // defined(HELPER_EXECUTABLE)

}  // namespace

__attribute__((visibility("default"))) int main(int argc, char* argv[]) {
  uint32_t exec_path_size = 0;
  int rv = _NSGetExecutablePath(NULL, &exec_path_size);
  if (rv != -1) {
    fprintf(stderr, "_NSGetExecutablePath: get length failed\n");
    abort();
  }

  char* exec_path = new char[exec_path_size];
  rv = _NSGetExecutablePath(exec_path, &exec_path_size);
  if (rv != 0) {
    fprintf(stderr, "_NSGetExecutablePath: get path failed\n");
    abort();
  }

#if defined(HELPER_EXECUTABLE)
  bool enable_v2_sandbox = false;
  int fd_mapping = -1;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], v2_sandbox_arg) == 0) {
      enable_v2_sandbox = true;
    } else if (strncmp(argv[i], fd_mapping_arg, strlen(fd_mapping_arg)) == 0) {
      // Parse --fd_mapping=X to get the file descriptor X.
      std::string arg(argv[i]);
      std::string fd_str = arg.substr(strlen(fd_mapping_arg), arg.length());
      fd_mapping = std::stoi(fd_str);
    }
  }
  if (enable_v2_sandbox && fd_mapping == -1) {
    fprintf(stderr, "Must pass a valid file descriptor to --fd_mapping.\n");
    abort();
  }

  // SandboxInit enables the sandbox and then returns.
  if (enable_v2_sandbox)
    SandboxInit(exec_path, argc, argv, fd_mapping);

  const char* const rel_path =
      "../../../" PRODUCT_FULLNAME_STRING
      " Framework.framework/" PRODUCT_FULLNAME_STRING " Framework";
#else
  const char* const rel_path =
      "../Versions/" CHROME_VERSION_STRING "/" PRODUCT_FULLNAME_STRING
      " Framework.framework/" PRODUCT_FULLNAME_STRING " Framework";
#endif  // defined(HELPER_EXECUTABLE)

  // Slice off the last part of the main executable path, and append the
  // version framework information.
  const char* parent_dir = dirname(exec_path);
  if (!parent_dir) {
    fprintf(stderr, "dirname %s: %s\n", exec_path, strerror(errno));
    abort();
  }
  delete[] exec_path;

  const size_t parent_dir_len = strlen(parent_dir);
  const size_t rel_path_len = strlen(rel_path);
  // 2 accounts for a trailing NUL byte and the '/' in the middle of the paths.
  const size_t framework_path_size = parent_dir_len + rel_path_len + 2;
  char* framework_path = new char[framework_path_size];
  snprintf(framework_path, framework_path_size, "%s/%s", parent_dir, rel_path);

  void* library = dlopen(framework_path, RTLD_LAZY | RTLD_LOCAL | RTLD_FIRST);
  if (!library) {
    fprintf(stderr, "dlopen %s: %s\n", framework_path, dlerror());
    abort();
  }
  delete[] framework_path;

  const ChromeMainPtr chrome_main =
      reinterpret_cast<ChromeMainPtr>(dlsym(library, "ChromeMain"));
  if (!chrome_main) {
    fprintf(stderr, "dlsym ChromeMain: %s\n", dlerror());
    abort();
  }
  rv = chrome_main(argc, argv);

  // exit, don't return from main, to avoid the apparent removal of main from
  // stack backtraces under tail call optimization.
  exit(rv);
}
