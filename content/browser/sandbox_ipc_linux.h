// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// https://chromium.googlesource.com/chromium/src/+/master/docs/linux_sandbox_ipc.md

#ifndef CONTENT_BROWSER_SANDBOX_IPC_LINUX_H_
#define CONTENT_BROWSER_SANDBOX_IPC_LINUX_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/pickle.h"
#include "base/threading/simple_thread.h"
#include "content/common/content_export.h"
#include "third_party/icu/source/common/unicode/uchar.h"

class SkString;

namespace content {

class SandboxIPCHandler : public base::DelegateSimpleThread::Delegate {
 public:
  // lifeline_fd: the read end of a pipe which the main thread holds
  // the other end of.
  // browser_socket: the browser's end of the sandbox IPC socketpair.
  SandboxIPCHandler(int lifeline_fd, int browser_socket);
  ~SandboxIPCHandler() override;

  void Run() override;

  class TestObserver {
   public:
    virtual void OnGetFallbackFontForChar(UChar32 c,
                                          std::string name,
                                          int id) = 0;
    virtual void OnFontOpen(int id) = 0;
  };
  CONTENT_EXPORT static void SetObserverForTests(TestObserver* observer);

 private:
  int FindOrAddPath(const SkString& path);

  void HandleRequestFromChild(int fd);

  void HandleFontMatchRequest(int fd,
                              base::PickleIterator iter,
                              const std::vector<base::ScopedFD>& fds);

  void HandleFontOpenRequest(int fd,
                             base::PickleIterator iter,
                             const std::vector<base::ScopedFD>& fds);

  void HandleGetFallbackFontForChar(int fd,
                                    base::PickleIterator iter,
                                    const std::vector<base::ScopedFD>& fds);

  void HandleGetStyleForStrike(int fd,
                               base::PickleIterator iter,
                               const std::vector<base::ScopedFD>& fds);

  void HandleLocaltime(int fd,
                       base::PickleIterator iter,
                       const std::vector<base::ScopedFD>& fds);

  void HandleMakeSharedMemorySegment(int fd,
                                     base::PickleIterator iter,
                                     const std::vector<base::ScopedFD>& fds);

  void HandleMatchWithFallback(int fd,
                               base::PickleIterator iter,
                               const std::vector<base::ScopedFD>& fds);

  void SendRendererReply(const std::vector<base::ScopedFD>& fds,
                         const base::Pickle& reply,
                         int reply_fd);

  const int lifeline_fd_;
  const int browser_socket_;
  std::vector<SkString> paths_;

  DISALLOW_COPY_AND_ASSIGN(SandboxIPCHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_SANDBOX_IPC_LINUX_H_
