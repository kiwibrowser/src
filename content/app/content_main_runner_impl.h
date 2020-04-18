// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_APP_CONTENT_MAIN_RUNNER_IMPL_H_
#define CONTENT_APP_CONTENT_MAIN_RUNNER_IMPL_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "build/build_config.h"
#include "content/public/app/content_main.h"
#include "content/public/app/content_main_runner.h"
#include "content/public/common/content_client.h"

#if defined(OS_WIN)
#include "sandbox/win/src/sandbox_types.h"
#elif defined(OS_MACOSX)
#include "base/mac/scoped_nsautorelease_pool.h"
#endif  // OS_WIN

namespace base {
class AtExitManager;
class SingleThreadTaskRunner;
}  // namespace base

namespace content {
class BrowserProcessSubThread;
class ContentMainDelegate;
struct ContentMainParams;

class ContentMainRunnerImpl : public ContentMainRunner {
 public:
  static ContentMainRunnerImpl* Create();

  ContentMainRunnerImpl();
  ~ContentMainRunnerImpl() override;

  int TerminateForFatalInitializationError();

  // ContentMainRunner:
  int Initialize(const ContentMainParams& params) override;
  int Run() override;
  void Shutdown() override;

#if !defined(CHROME_MULTIPLE_DLL_CHILD)
  // Creates a thread and returns the SingleThreadTaskRunner on which
  // ServiceManager should run.
  scoped_refptr<base::SingleThreadTaskRunner>
  GetServiceManagerTaskRunnerForEmbedderProcess();
#endif  // !defined(CHROME_MULTIPLE_DLL_CHILD)

 private:
  // True if the runner has been initialized.
  bool is_initialized_ = false;

  // True if the runner has been shut down.
  bool is_shutdown_ = false;

  // True if basic startup was completed.
  bool completed_basic_startup_ = false;

  // Used if the embedder doesn't set one.
  ContentClient empty_content_client_;

  // The delegate will outlive this object.
  ContentMainDelegate* delegate_ = nullptr;

  std::unique_ptr<base::AtExitManager> exit_manager_;

#if defined(OS_WIN)
  sandbox::SandboxInterfaceInfo sandbox_info_;
#elif defined(OS_MACOSX)
  base::mac::ScopedNSAutoreleasePool* autorelease_pool_ = nullptr;
#endif

  std::unique_ptr<BrowserProcessSubThread> service_manager_thread_;

  base::Closure* ui_task_ = nullptr;

  CreatedMainPartsClosure* created_main_parts_closure_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ContentMainRunnerImpl);
};

}  // namespace content

#endif  // CONTENT_APP_CONTENT_MAIN_RUNNER_IMPL_H_
