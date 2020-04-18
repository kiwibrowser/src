// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/win/audio_device_listener_win.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/win/scoped_com_initializer.h"
#include "media/audio/audio_manager.h"
#include "media/audio/audio_unittest_util.h"
#include "media/audio/win/core_audio_util_win.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::win::ScopedCOMInitializer;

namespace media {

static const char kFirstTestDevice[] = "test_device_0";
static const char kSecondTestDevice[] = "test_device_1";

class AudioDeviceListenerWinTest : public testing::Test {
 public:
  AudioDeviceListenerWinTest() {
    DCHECK(com_init_.Succeeded());
  }

  virtual void SetUp() {
    if (!CoreAudioUtil::IsSupported())
      return;

    output_device_listener_.reset(new AudioDeviceListenerWin(base::Bind(
        &AudioDeviceListenerWinTest::OnDeviceChange, base::Unretained(this))));

    tick_clock_.Advance(base::TimeDelta::FromSeconds(12345));
    output_device_listener_->tick_clock_ = &tick_clock_;
  }

  void AdvanceLastDeviceChangeTime() {
    tick_clock_.Advance(base::TimeDelta::FromMilliseconds(
        AudioDeviceListenerWin::kDeviceChangeLimitMs + 1));
  }

  // Simulate a device change where no output devices are available.
  bool SimulateNullDefaultOutputDeviceChange() {
    return output_device_listener_->OnDefaultDeviceChanged(
        static_cast<EDataFlow>(eConsole), static_cast<ERole>(eRender),
        NULL) == S_OK;
  }

  bool SimulateDefaultOutputDeviceChange(const char* new_device_id) {
    return output_device_listener_->OnDefaultDeviceChanged(
        static_cast<EDataFlow>(eConsole), static_cast<ERole>(eRender),
        base::ASCIIToUTF16(new_device_id).c_str()) == S_OK;
  }


  MOCK_METHOD0(OnDeviceChange, void());

 private:
  ScopedCOMInitializer com_init_;
  std::unique_ptr<AudioDeviceListenerWin> output_device_listener_;
  base::SimpleTestTickClock tick_clock_;

  DISALLOW_COPY_AND_ASSIGN(AudioDeviceListenerWinTest);
};

// Simulate a device change events and ensure we get the right callbacks.
TEST_F(AudioDeviceListenerWinTest, OutputDeviceChange) {
  ABORT_AUDIO_TEST_IF_NOT(CoreAudioUtil::IsSupported());

  EXPECT_CALL(*this, OnDeviceChange()).Times(1);
  ASSERT_TRUE(SimulateDefaultOutputDeviceChange(kFirstTestDevice));

  testing::Mock::VerifyAndClear(this);
  AdvanceLastDeviceChangeTime();
  EXPECT_CALL(*this, OnDeviceChange()).Times(1);
  ASSERT_TRUE(SimulateDefaultOutputDeviceChange(kSecondTestDevice));

  // The second device event should be ignored since it occurs too soon.
  ASSERT_TRUE(SimulateDefaultOutputDeviceChange(kSecondTestDevice));
}

// Ensure that null output device changes don't crash.  Simulates the situation
// where we have no output devices.
TEST_F(AudioDeviceListenerWinTest, NullOutputDeviceChange) {
  ABORT_AUDIO_TEST_IF_NOT(CoreAudioUtil::IsSupported());

  EXPECT_CALL(*this, OnDeviceChange()).Times(1);
  ASSERT_TRUE(SimulateNullDefaultOutputDeviceChange());

  testing::Mock::VerifyAndClear(this);
  AdvanceLastDeviceChangeTime();
  EXPECT_CALL(*this, OnDeviceChange()).Times(1);
  ASSERT_TRUE(SimulateDefaultOutputDeviceChange(kFirstTestDevice));

  testing::Mock::VerifyAndClear(this);
  AdvanceLastDeviceChangeTime();
  EXPECT_CALL(*this, OnDeviceChange()).Times(1);
  ASSERT_TRUE(SimulateNullDefaultOutputDeviceChange());
}

}  // namespace media
