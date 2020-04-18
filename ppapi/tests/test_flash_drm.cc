// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_flash_drm.h"

#include <stdint.h>

#if defined(PPAPI_OS_WIN)
#include <Windows.h>
#endif

#include "ppapi/c/pp_macros.h"
#include "ppapi/c/private/ppb_file_ref_private.h"
#include "ppapi/c/private/ppb_flash_drm.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/private/flash_device_id.h"
#include "ppapi/cpp/private/flash_drm.h"
#include "ppapi/cpp/var.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(FlashDRM);

using pp::flash::DeviceID;
using pp::flash::DRM;
using pp::FileRef;
using pp::PassRef;
using pp::Var;

namespace {

const char kExepectedVoucherFilename[] = "plugin.vch";

// Check that the Hmonitor value is what it is expected to be for each platform.
bool CheckExpectedHmonitorValue(bool success, int64_t hmonitor) {
#if defined(PPAPI_OS_MACOSX)
  // TODO(raymes): Verify the expected |hmonitor| value on Mac.
  return success;
#elif defined(PPAPI_OS_WIN)
  MONITORINFO info = { sizeof(info) };
  return success &&
      ::GetMonitorInfo(reinterpret_cast<HMONITOR>(hmonitor), &info) == TRUE;
#else
  // Not implemented for other platforms, should return false.
  return !success;
#endif
}

}  // namespace

TestFlashDRM::TestFlashDRM(TestingInstance* instance)
    : TestCase(instance),
      callback_factory_(this) {
}

void TestFlashDRM::RunTests(const std::string& filter) {
  RUN_TEST(GetDeviceID, filter);
  RUN_TEST(GetHmonitor, filter);
  RUN_TEST(GetVoucherFile, filter);
}

std::string TestFlashDRM::TestGetDeviceID() {
  // Test the old C++ wrapper.
  // TODO(raymes): Remove this once Flash switches APIs.
  {
    DeviceID device_id(instance_);
    TestCompletionCallbackWithOutput<Var> output_callback(
        instance_->pp_instance());
    int32_t rv = device_id.GetDeviceID(output_callback.GetCallback());
    output_callback.WaitForResult(rv);
    ASSERT_TRUE(output_callback.result() == PP_OK);
    Var result = output_callback.output();
    ASSERT_TRUE(result.is_string());
    std::string id = result.AsString();
    ASSERT_FALSE(id.empty());
  }

  {
    DRM drm(instance_);
    TestCompletionCallbackWithOutput<Var> output_callback(
        instance_->pp_instance());
    int32_t rv = drm.GetDeviceID(output_callback.GetCallback());
    output_callback.WaitForResult(rv);
    ASSERT_TRUE(output_callback.result() == PP_OK);
    Var result = output_callback.output();
    ASSERT_TRUE(result.is_string());
    std::string id = result.AsString();
    ASSERT_FALSE(id.empty());
  }

  PASS();
}

std::string TestFlashDRM::TestGetHmonitor() {
  DRM drm(instance_);
  int64_t hmonitor;
  while (true) {
    bool success = drm.GetHmonitor(&hmonitor);
    if (CheckExpectedHmonitorValue(success, hmonitor))
      break;
    else
      PlatformSleep(30);
  }

  PASS();
}

std::string TestFlashDRM::TestGetVoucherFile() {
  DRM drm(instance_);
  TestCompletionCallbackWithOutput<FileRef> output_callback(
      instance_->pp_instance());
  int32_t rv = drm.GetVoucherFile(output_callback.GetCallback());
  output_callback.WaitForResult(rv);
  ASSERT_EQ(PP_OK, output_callback.result());
  FileRef result = output_callback.output();
  ASSERT_EQ(PP_FILESYSTEMTYPE_EXTERNAL, result.GetFileSystemType());

  // The PPB_FileRefPrivate interface doesn't have a C++ wrapper yet, so just
  // use the C interface.
  const PPB_FileRefPrivate* file_ref_private =
      static_cast<const PPB_FileRefPrivate*>(
          pp::Module::Get()->GetBrowserInterface(PPB_FILEREFPRIVATE_INTERFACE));
  ASSERT_TRUE(file_ref_private);
  Var path(PassRef(), file_ref_private->GetAbsolutePath(result.pp_resource()));
  ASSERT_TRUE(path.is_string());
  std::string path_string = path.AsString();
  std::string expected_filename = std::string(kExepectedVoucherFilename);
  ASSERT_EQ(expected_filename,
            path_string.substr(path_string.size() - expected_filename.size()));

  PASS();
}
