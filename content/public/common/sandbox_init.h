// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SANDBOX_INIT_H_
#define CONTENT_PUBLIC_COMMON_SANDBOX_INIT_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/files/scoped_file.h"
#include "base/memory/shared_memory.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "build/build_config.h"
#include "content/common/content_export.h"
#include "services/service_manager/sandbox/sandbox_type.h"

namespace base {
class CommandLine;
}

namespace sandbox {
namespace bpf_dsl {
class Policy;
}
struct SandboxInterfaceInfo;
enum ResultCode : int;
}

namespace content {
class SandboxedProcessLauncherDelegate;

#if defined(OS_WIN)

// Initialize the sandbox of the given |sandbox_type|. Although The browser
// process is not sandboxed, this also needs to be called because it will
// initialize the broker code.
//
// Returns true if the sandbox was initialized succesfully, false if an error
// occurred.  If process_type isn't one that needs sandboxing true is always
// returned.
CONTENT_EXPORT bool InitializeSandbox(
    service_manager::SandboxType sandbox_type,
    sandbox::SandboxInterfaceInfo* sandbox_info);

// Launch a sandboxed process. |delegate| may be NULL. If |delegate| is non-NULL
// then it just has to outlive this method call. |handles_to_inherit| is a list
// of handles for the child process to inherit. The caller retains ownership of
// the handles.
CONTENT_EXPORT sandbox::ResultCode StartSandboxedProcess(
    SandboxedProcessLauncherDelegate* delegate,
    base::CommandLine* child_command_line,
    const base::HandlesToInheritVector& handles_to_inherit,
    base::Process* process);

#elif defined(OS_MACOSX)

// Initialize the sandbox of the given |sandbox_type|, optionally specifying a
// directory to allow access to. Note specifying a directory needs to be
// supported by the sandbox profile associated with the given |sandbox_type|.
//
// Returns true if the sandbox was initialized succesfully, false if an error
// occurred.  If process_type isn't one that needs sandboxing, no action is
// taken and true is always returned.
CONTENT_EXPORT bool InitializeSandbox(
    service_manager::SandboxType sandbox_type);

// Initialize the sandbox for renderer, gpu, utility, worker, and plugin
// processes, depending on the command line flags. For the browser process which
// is not sandboxed, this call is a no-op.
// Returns true if the sandbox was initialized succesfully, false if an error
// occurred.  If process_type isn't one that needs sandboxing, true is always
// returned.
CONTENT_EXPORT bool InitializeSandbox();

// Initializes the sandbox, as described above, but executes the callback after
// warmup and before initialization.
CONTENT_EXPORT bool InitializeSandbox(base::OnceClosure post_warmup_hook);

#elif defined(OS_LINUX) || defined(OS_NACL_NONSFI)

// Initialize a seccomp-bpf sandbox. |policy| may not be NULL.
// If an existing layer of sandboxing is present that would prevent access to
// /proc, |proc_fd| must be a valid file descriptor to /proc/.
// Returns true if the sandbox has been properly engaged.
CONTENT_EXPORT bool InitializeSandbox(
    std::unique_ptr<sandbox::bpf_dsl::Policy> policy,
    base::ScopedFD proc_fd);

// Return a "baseline" policy. This is used by other modules to implement a
// policy that is derived from the baseline.
CONTENT_EXPORT std::unique_ptr<sandbox::bpf_dsl::Policy>
GetBPFSandboxBaselinePolicy();
#endif  // defined(OS_LINUX) || defined(OS_NACL_NONSFI)

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_SANDBOX_INIT_H_
