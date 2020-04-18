// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_CHILD_PROCESS_HOST_H_
#define CONTENT_PUBLIC_COMMON_CHILD_PROCESS_HOST_H_

#include <stdint.h>

#include "base/files/scoped_file.h"
#include "build/build_config.h"
#include "content/common/content_export.h"
#include "content/public/common/bind_interface_helpers.h"
#include "ipc/ipc_channel_proxy.h"

namespace base {
class FilePath;
}

namespace IPC {
class MessageFilter;
}

namespace content {

class ChildProcessHostDelegate;

// This represents a non-browser process. This can include traditional child
// processes like plugins, or an embedder could even use this for long lived
// processes that run independent of the browser process.
class CONTENT_EXPORT ChildProcessHost : public IPC::Sender {
 public:
  ~ChildProcessHost() override {}

  // This is a value never returned as the unique id of any child processes of
  // any kind, including the values returned by RenderProcessHost::GetID().
  enum : int { kInvalidUniqueID = -1 };

  // Used to create a child process host. The delegate must outlive this object.
  static ChildProcessHost* Create(ChildProcessHostDelegate* delegate);

  // These flags may be passed to GetChildPath in order to alter its behavior,
  // causing it to return a child path more suited to a specific task.
  enum {
    // No special behavior requested.
    CHILD_NORMAL = 0,

#if defined(OS_LINUX)
    // Indicates that the child execed after forking may be execced from
    // /proc/self/exe rather than using the "real" app path. This prevents
    // autoupdate from confusing us if it changes the file out from under us.
    // You will generally want to set this on Linux, except when there is an
    // override to the command line (for example, we're forking a renderer in
    // gdb). In this case, you'd use GetChildPath to get the real executable
    // file name, and then prepend the GDB command to the command line.
    CHILD_ALLOW_SELF = 1 << 0,
#endif  // defined(OS_LINUX)
  };

  // Returns the pathname to be used for a child process.  If a subprocess
  // pathname was specified on the command line, that will be used.  Otherwise,
  // the default child process pathname will be returned.  On most platforms,
  // this will be the same as the currently-executing process.
  //
  // The |flags| argument accepts one or more flags such as CHILD_ALLOW_SELF.
  // Pass only CHILD_NORMAL if none of these special behaviors are required.
  //
  // On failure, returns an empty FilePath.
  static base::FilePath GetChildPath(int flags);

  // Send the shutdown message to the child process.
  virtual void ForceShutdown() = 0;

  // Creates the IPC channel over a Mojo message pipe. The pipe connection is
  // brokered through the Service Manager like any other service connection.
  virtual void CreateChannelMojo() = 0;

  // Returns true iff the IPC channel is currently being opened;
  virtual bool IsChannelOpening() = 0;

  // Adds an IPC message filter.  A reference will be kept to the filter.
  virtual void AddFilter(IPC::MessageFilter* filter) = 0;

  // Bind an interface exposed by the child process.
  virtual void BindInterface(const std::string& interface_name,
                             mojo::ScopedMessagePipeHandle interface_pipe) = 0;
};

};  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_CHILD_PROCESS_HOST_H_
