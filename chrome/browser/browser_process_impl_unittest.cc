// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process_impl.h"

#include <memory>

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/user_metrics.h"
#include "base/run_loop.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/webui/ntp/ntp_resource_cache_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread.h"
#include "content/public/test/test_service_manager_context.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
#include "base/win/scoped_com_initializer.h"
#endif

class BrowserProcessImplTest : public ::testing::Test {
 protected:
  BrowserProcessImplTest()
      : stashed_browser_process_(g_browser_process),
        loop_(base::MessageLoop::TYPE_UI),
        ui_thread_(content::BrowserThread::UI, &loop_),
        command_line_(base::CommandLine::NO_PROGRAM),
        browser_process_impl_(
            new BrowserProcessImpl(base::ThreadTaskRunnerHandle::Get().get())) {
    // Create() and StartWithDefaultParams() TaskScheduler in seperate steps to
    // properly simulate the browser process' lifecycle.
    base::TaskScheduler::Create("BrowserProcessImplTest");
    base::SetRecordActionTaskRunner(loop_.task_runner());
    browser_process_impl_->SetApplicationLocale("en");
  }

  ~BrowserProcessImplTest() override {
    g_browser_process = nullptr;
    browser_process_impl_.reset();
    // Restore the original browser process.
    g_browser_process = stashed_browser_process_;
  }

  // Creates the IO thread (unbound) and task scheduler threads. The UI thread
  // needs to be alive while BrowserProcessImpl is alive, and is managed
  // separately.
  void StartSecondaryThreads() {
    base::TaskScheduler::GetInstance()->StartWithDefaultParams();

    io_thread_ = std::make_unique<content::TestBrowserThread>(
        content::BrowserThread::IO);
    io_thread_->StartIOThreadUnregistered();
  }

  // Binds the IO thread to BrowserThread::IO and starts the ServiceManager.
  void Initialize() {
    io_thread_->RegisterAsBrowserThread();

    // TestServiceManagerContext creation requires the task scheduler to be
    // started.
    service_manager_context_ =
        std::make_unique<content::TestServiceManagerContext>();
  }

  // Destroys the secondary threads (all threads except the UI thread).
  // The UI thread needs to be alive while BrowserProcessImpl is alive, and is
  // managed separately.
  void DestroySecondaryThreads() {
    // TestServiceManagerContext must be destroyed before the IO thread.
    service_manager_context_.reset();
    // Spin the runloop to allow posted tasks to be processed.
    base::RunLoop().RunUntilIdle();
    io_thread_.reset();
    base::RunLoop().RunUntilIdle();
    base::TaskScheduler::GetInstance()->Shutdown();
    base::TaskScheduler::GetInstance()->JoinForTesting();
    base::TaskScheduler::SetInstance(nullptr);
  }

  BrowserProcessImpl* browser_process_impl() {
    return browser_process_impl_.get();
  }

  base::CommandLine* command_line() { return &command_line_; }

 private:
  BrowserProcess* stashed_browser_process_;
  base::MessageLoop loop_;
  content::TestBrowserThread ui_thread_;
#if defined(OS_WIN)
  base::win::ScopedCOMInitializer scoped_com_initializer_;
#endif
  std::unique_ptr<content::TestBrowserThread> io_thread_;
  base::CommandLine command_line_;
  std::unique_ptr<content::TestServiceManagerContext> service_manager_context_;
  std::unique_ptr<BrowserProcessImpl> browser_process_impl_;
};


// Android does not have the NTPResourceCache.
// This test crashes on ChromeOS because it relies on NetworkHandler which
// cannot be used in test.
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
// TODO(crbug.com/807386): Flaky on Linux TSAN.
#if defined(OS_LINUX)
#define MAYBE_LifeCycle DISABLED_LifeCycle
#else
#define MAYBE_LifeCycle LifeCycle
#endif
TEST_F(BrowserProcessImplTest, MAYBE_LifeCycle) {
  // Setup the BrowserProcessImpl and the threads.
  browser_process_impl()->Init();
  // A lightweigh IO Thread is created before the
  // BrowserThreadImpl::PreCreateThreads is called. https://crbug.com/729596.
  StartSecondaryThreads();
  browser_process_impl()->PreCreateThreads(*command_line());
  Initialize();
  browser_process_impl()->PreMainMessageLoopRun();

  // Force the creation of the NTPResourceCache, to test the destruction order.
  std::unique_ptr<Profile> profile(new TestingProfile);
  NTPResourceCache* cache =
      NTPResourceCacheFactory::GetForProfile(profile.get());
  ASSERT_TRUE(cache);
  // Pass ownership to the ProfileManager so that it manages the destruction.
  browser_process_impl()->profile_manager()->RegisterTestingProfile(
      profile.release(), false, false);

  // Tear down the BrowserProcessImpl and the threads.
  browser_process_impl()->StartTearDown();
  DestroySecondaryThreads();
  browser_process_impl()->PostDestroyThreads();
}
#endif  // !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
