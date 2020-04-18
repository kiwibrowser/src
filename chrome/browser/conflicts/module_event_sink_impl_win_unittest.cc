// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/module_event_sink_impl_win.h"

#include <memory>

#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/conflicts/module_database_win.h"
#include "chrome/common/conflicts/module_watcher_win.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

#include <windows.h>

namespace {

// The address of this module in memory. The linker will take care of defining
// this symbol.
extern "C" IMAGE_DOS_HEADER __ImageBase;

// An invalid load address.
const uint64_t kInvalidLoadAddress = 0xDEADBEEF;

}  // namespace

class ModuleEventSinkImplTest : public testing::Test {
 protected:
  ModuleEventSinkImplTest()
      : scoped_testing_local_state_(TestingBrowserProcess::GetGlobal()),
        module_database_(std::make_unique<ModuleDatabase>(
            base::SequencedTaskRunnerHandle::Get())) {}

  bool CreateModuleSinkImpl() {
    HANDLE process_handle = 0;
    if (!::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentProcess(),
                           ::GetCurrentProcess(), &process_handle,

                                       0,
                                       FALSE,
                                       DUPLICATE_SAME_ACCESS)) {
      return false;
    }

    module_event_sink_impl_ = std::make_unique<ModuleEventSinkImpl>(
        base::Process(process_handle), content::PROCESS_TYPE_BROWSER,
        module_database_.get());
    return true;
  }

  const ModuleDatabase::ModuleMap& modules() {
    return module_database_->modules_;
  }

  // Must be before |module_database_|.
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  ScopedTestingLocalState scoped_testing_local_state_;
  std::unique_ptr<ModuleDatabase> module_database_;
  std::unique_ptr<ModuleEventSinkImpl> module_event_sink_impl_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ModuleEventSinkImplTest);
};

TEST_F(ModuleEventSinkImplTest, CallsForwardedAsExpected) {
  const uintptr_t kValidLoadAddress = reinterpret_cast<uintptr_t>(&__ImageBase);

  EXPECT_EQ(0u, modules().size());

  ASSERT_TRUE(CreateModuleSinkImpl());
  EXPECT_EQ(0u, modules().size());

  // An invalid load event should not cause a module entry.
  module_event_sink_impl_->OnModuleEvent(
      mojom::ModuleEventType::MODULE_ALREADY_LOADED, kInvalidLoadAddress);
  test_browser_thread_bundle_.RunUntilIdle();
  EXPECT_EQ(0u, modules().size());

  // A valid load event should cause a module entry.
  module_event_sink_impl_->OnModuleEvent(mojom::ModuleEventType::MODULE_LOADED,
                                         kValidLoadAddress);
  test_browser_thread_bundle_.RunUntilIdle();
  EXPECT_EQ(1u, modules().size());
}
