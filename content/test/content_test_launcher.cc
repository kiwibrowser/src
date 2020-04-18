// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_launcher.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/i18n/icu_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/process/memory.h"
#include "base/sys_info.h"
#include "base/test/launcher/test_launcher.h"
#include "base/test/test_suite.h"
#include "base/test/test_timeouts.h"
#include "build/build_config.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/content_test_suite_base.h"
#include "content/shell/app/shell_main_delegate.h"
#include "content/shell/common/shell_switches.h"
#include "media/base/media_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/ui_base_switches.h"
#include "ui/base/ui_features.h"

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
#include "gin/v8_initializer.h"
#endif

#if defined(OS_ANDROID)
#include "base/message_loop/message_loop.h"
#include "content/app/mojo/mojo_init.h"
#include "content/common/url_schemes.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/nested_message_pump_android.h"
#include "content/shell/browser/shell_content_browser_client.h"
#include "content/shell/common/shell_content_client.h"
#include "ui/base/ui_base_paths.h"
#endif

namespace content {

#if defined(OS_ANDROID)
std::unique_ptr<base::MessagePump> CreateMessagePumpForUI() {
  return std::unique_ptr<base::MessagePump>(new NestedMessagePumpAndroid());
};
#endif

class ContentBrowserTestSuite : public ContentTestSuiteBase {
 public:
  ContentBrowserTestSuite(int argc, char** argv)
      : ContentTestSuiteBase(argc, argv) {
  }
  ~ContentBrowserTestSuite() override {}

 protected:
  void Initialize() override {
#if defined(OS_ANDROID)
    base::i18n::AllowMultipleInitializeCallsForTesting();
    base::i18n::InitializeICU();

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
    gin::V8Initializer::LoadV8Snapshot();
    gin::V8Initializer::LoadV8Natives();
#endif

    // This needs to be done before base::TestSuite::Initialize() is called,
    // as it also tries to set MessagePumpForUIFactory.
    if (!base::MessageLoop::InitMessagePumpForUIFactory(
            &CreateMessagePumpForUI))
      VLOG(0) << "MessagePumpForUIFactory already set, unable to override.";

    // For all other platforms, we call ContentMain for browser tests which goes
    // through the normal browser initialization paths. For Android, we must set
    // things up manually.
    content_client_.reset(new ShellContentClient);
    browser_content_client_.reset(new ShellContentBrowserClient());
    SetContentClient(content_client_.get());
    SetBrowserClientForTesting(browser_content_client_.get());

    content::RegisterContentSchemes(false);
    RegisterPathProvider();
    ui::RegisterPathProvider();
    RegisterInProcessThreads();

    InitializeMojo();
#endif

    ContentTestSuiteBase::Initialize();
  }

#if defined(OS_ANDROID)
  std::unique_ptr<ShellContentClient> content_client_;
  std::unique_ptr<ShellContentBrowserClient> browser_content_client_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ContentBrowserTestSuite);
};

class ContentTestLauncherDelegate : public TestLauncherDelegate {
 public:
  ContentTestLauncherDelegate() {}
  ~ContentTestLauncherDelegate() override {}

  int RunTestSuite(int argc, char** argv) override {
    return ContentBrowserTestSuite(argc, argv).Run();
  }

  bool AdjustChildProcessCommandLine(
      base::CommandLine* command_line,
      const base::FilePath& temp_data_dir) override {
    command_line->AppendSwitchPath(switches::kContentShellDataPath,
                                   temp_data_dir);
    command_line->AppendSwitch(switches::kUseFakeDeviceForMediaStream);
    return true;
  }

 protected:
  ContentMainDelegate* CreateContentMainDelegate() override {
    return new ShellMainDelegate(true);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ContentTestLauncherDelegate);
};

}  // namespace content

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  size_t parallel_jobs = base::NumParallelJobs();
  if (parallel_jobs > 1U) {
    parallel_jobs /= 2U;
  }
  content::ContentTestLauncherDelegate launcher_delegate;
  return LaunchTests(&launcher_delegate, parallel_jobs, argc, argv);
}
