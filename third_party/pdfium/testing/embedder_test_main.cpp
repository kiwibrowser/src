// Copyright 2018 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "core/fxcrt/fx_memory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/test_support.h"

#ifdef PDF_ENABLE_V8
#include "v8/include/v8-platform.h"
#include "v8/include/v8.h"
#endif  // PDF_ENABLE_v8

namespace {

const char* g_exe_path = nullptr;

#ifdef PDF_ENABLE_V8
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
v8::StartupData* g_v8_natives = nullptr;
v8::StartupData* g_v8_snapshot = nullptr;
#endif  // V8_USE_EXTERNAL_STARTUP_DATA
#endif  // PDF_ENABLE_V8

// The loading time of the CFGAS_FontMgr is linear in the number of times it is
// loaded. So, if a test suite has a lot of tests that need a font manager they
// can end up executing very, very slowly.
class Environment : public testing::Environment {
 public:
  void SetUp() override {
#ifdef PDF_ENABLE_V8
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
    if (g_v8_natives && g_v8_snapshot) {
      platform_ = InitializeV8ForPDFiumWithStartupData(
          g_exe_path, std::string(), nullptr, nullptr);
    } else {
      g_v8_natives = new v8::StartupData;
      g_v8_snapshot = new v8::StartupData;
      platform_ = InitializeV8ForPDFiumWithStartupData(
          g_exe_path, std::string(), g_v8_natives, g_v8_snapshot);
    }
#else
    platform_ = InitializeV8ForPDFium(g_exe_path);
#endif  // V8_USE_EXTERNAL_STARTUP_DATA
#endif  // FPDF_ENABLE_V8
  }

  void TearDown() override {
#ifdef PDF_ENABLE_V8
    v8::V8::ShutdownPlatform();
#endif  // PDF_ENABLE_V8
  }

 private:
#ifdef PDF_ENABLE_V8
  std::unique_ptr<v8::Platform> platform_;
#endif  // PDF_ENABLE_V8
};

Environment* env_ = nullptr;

}  // namespace

// Can't use gtest-provided main since we need to stash the path to the
// executable in order to find the external V8 binary data files.
int main(int argc, char** argv) {
  g_exe_path = argv[0];

  FXMEM_InitializePartitionAlloc();

  env_ = new Environment();
  // The env will be deleted by gtest.
  AddGlobalTestEnvironment(env_);

  testing::InitGoogleTest(&argc, argv);
  testing::InitGoogleMock(&argc, argv);

  int ret_val = RUN_ALL_TESTS();

#ifdef PDF_ENABLE_V8
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  if (g_v8_natives)
    free(const_cast<char*>(g_v8_natives->data));
  if (g_v8_snapshot)
    free(const_cast<char*>(g_v8_snapshot->data));
#endif  // V8_USE_EXTERNAL_STARTUP_DATA
#endif  // PDF_ENABLE_V8

  return ret_val;
}
