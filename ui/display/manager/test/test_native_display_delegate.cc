// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/manager/test/test_native_display_delegate.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/display/manager/test/action_logger.h"
#include "ui/display/types/display_mode.h"
#include "ui/display/types/native_display_observer.h"

namespace display {
namespace test {

TestNativeDisplayDelegate::TestNativeDisplayDelegate(ActionLogger* log)
    : max_configurable_pixels_(0),
      get_hdcp_expectation_(true),
      set_hdcp_expectation_(true),
      hdcp_state_(HDCP_STATE_UNDESIRED),
      run_async_(false),
      log_(log) {}

TestNativeDisplayDelegate::~TestNativeDisplayDelegate() {}

void TestNativeDisplayDelegate::Initialize() {
  log_->AppendAction(kInit);
}

void TestNativeDisplayDelegate::TakeDisplayControl(
    DisplayControlCallback callback) {
  log_->AppendAction(kTakeDisplayControl);
  std::move(callback).Run(true);
}

void TestNativeDisplayDelegate::RelinquishDisplayControl(
    DisplayControlCallback callback) {
  log_->AppendAction(kRelinquishDisplayControl);
  std::move(callback).Run(true);
}

void TestNativeDisplayDelegate::GetDisplays(GetDisplaysCallback callback) {
  // This mimics the behavior of Ozone DRM when new display state arrives.
  for (NativeDisplayObserver& observer : observers_)
    observer.OnDisplaySnapshotsInvalidated();

  if (run_async_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), outputs_));
  } else {
    std::move(callback).Run(outputs_);
  }
}

bool TestNativeDisplayDelegate::Configure(const DisplaySnapshot& output,
                                          const DisplayMode* mode,
                                          const gfx::Point& origin) {
  log_->AppendAction(GetCrtcAction(output, mode, origin));

  if (max_configurable_pixels_ == 0)
    return true;

  if (!mode)
    return false;

  return mode->size().GetArea() <= max_configurable_pixels_;
}

void TestNativeDisplayDelegate::Configure(const DisplaySnapshot& output,
                                          const DisplayMode* mode,
                                          const gfx::Point& origin,
                                          ConfigureCallback callback) {
  bool result = Configure(output, mode, origin);
  if (run_async_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), result));
  } else {
    std::move(callback).Run(result);
  }
}

void TestNativeDisplayDelegate::GetHDCPState(const DisplaySnapshot& output,
                                             GetHDCPStateCallback callback) {
  std::move(callback).Run(get_hdcp_expectation_, hdcp_state_);
}

void TestNativeDisplayDelegate::SetHDCPState(const DisplaySnapshot& output,
                                             HDCPState state,
                                             SetHDCPStateCallback callback) {
  log_->AppendAction(GetSetHDCPStateAction(output, state));
  std::move(callback).Run(set_hdcp_expectation_);
}

bool TestNativeDisplayDelegate::SetColorCorrection(
    const DisplaySnapshot& output,
    const std::vector<GammaRampRGBEntry>& degamma_lut,
    const std::vector<GammaRampRGBEntry>& gamma_lut,
    const std::vector<float>& correction_matrix) {
  log_->AppendAction(SetColorCorrectionAction(output, degamma_lut, gamma_lut,
                                              correction_matrix));
  return true;
}

void TestNativeDisplayDelegate::AddObserver(NativeDisplayObserver* observer) {
  observers_.AddObserver(observer);
}

void TestNativeDisplayDelegate::RemoveObserver(
    NativeDisplayObserver* observer) {
  observers_.RemoveObserver(observer);
}

FakeDisplayController* TestNativeDisplayDelegate::GetFakeDisplayController() {
  return nullptr;
}

}  // namespace test
}  // namespace display
