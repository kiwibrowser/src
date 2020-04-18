// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/module_inspector_win.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

base::FilePath GetKernel32DllFilePath() {
  std::unique_ptr<base::Environment> env = base::Environment::Create();
  std::string sysroot;
  EXPECT_TRUE(env->GetVar("SYSTEMROOT", &sysroot));

  base::FilePath path =
      base::FilePath::FromUTF8Unsafe(sysroot).Append(L"system32\\kernel32.dll");

  return path;
}

class ModuleInspectorTest : public testing::Test {
 protected:
  ModuleInspectorTest()
      : module_inspector_(base::Bind(&ModuleInspectorTest::OnModuleInspected,
                                     base::Unretained(this))) {}

  void AddModules(const std::vector<ModuleInfoKey>& modules) {
    for (const auto& module : modules)
      module_inspector_.AddModule(module);
  }

  // Callback for ModuleInspector.
  void OnModuleInspected(
      const ModuleInfoKey& module_key,
      std::unique_ptr<ModuleInspectionResult> inspection_result) {
    inspected_modules_.push_back(std::move(inspection_result));
  }

  const std::vector<std::unique_ptr<ModuleInspectionResult>>&
  inspected_modules() {
    return inspected_modules_;
  }

 protected:
  // A TestBrowserThreadBundle is required instead of a ScopedTaskEnvironment
  // because of AfterStartupTaskUtils (DCHECK for BrowserThread::UI).
  //
  // Must be before the ModuleInspector.
  content::TestBrowserThreadBundle test_browser_thread_bundle_;

 private:
  ModuleInspector module_inspector_;

  std::vector<std::unique_ptr<ModuleInspectionResult>> inspected_modules_;

  DISALLOW_COPY_AND_ASSIGN(ModuleInspectorTest);
};

}  // namespace

TEST_F(ModuleInspectorTest, OneModule) {
  AddModules({
      {GetKernel32DllFilePath(), 0, 0, 1},
  });

  test_browser_thread_bundle_.RunUntilIdle();

  ASSERT_EQ(1u, inspected_modules().size());
  EXPECT_TRUE(inspected_modules().front());
}

TEST_F(ModuleInspectorTest, MultipleModules) {
  AddModules({
      {base::FilePath(), 0, 0, 1},
      {base::FilePath(), 0, 0, 2},
      {base::FilePath(), 0, 0, 3},
      {base::FilePath(), 0, 0, 4},
      {base::FilePath(), 0, 0, 5},
  });

  test_browser_thread_bundle_.RunUntilIdle();

  EXPECT_EQ(5u, inspected_modules().size());
  for (const auto& inspection_result : inspected_modules())
    EXPECT_TRUE(inspection_result);
}
