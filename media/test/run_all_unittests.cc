// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/command_line.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "base/test/test_suite.h"
#include "build/build_config.h"
#include "media/base/fake_localized_strings.h"
#include "media/base/media.h"
#include "media/base/media_switches.h"
#include "mojo/edk/embedder/embedder.h"

#if defined(OS_ANDROID)
#include "media/base/android/media_codec_util.h"
#endif

class TestSuiteNoAtExit : public base::TestSuite {
 public:
  TestSuiteNoAtExit(int argc, char** argv) : TestSuite(argc, argv) {}
  ~TestSuiteNoAtExit() override = default;

 protected:
  void Initialize() override;

 private:
  base::TestDiscardableMemoryAllocator discardable_memory_allocator_;
};

void TestSuiteNoAtExit::Initialize() {
  // Run TestSuite::Initialize first so that logging is initialized.
  base::TestSuite::Initialize();

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(switches::kEnableInbandTextTracks);

#if defined(OS_ANDROID)
  if (media::MediaCodecUtil::IsMediaCodecAvailable())
    media::EnablePlatformDecoderSupport();
#endif

  // Run this here instead of main() to ensure an AtExitManager is already
  // present.
  media::InitializeMediaLibrary();
  media::SetUpFakeLocalizedStrings();

  base::DiscardableMemoryAllocator::SetInstance(&discardable_memory_allocator_);
}

int main(int argc, char** argv) {
  mojo::edk::Init();
  TestSuiteNoAtExit test_suite(argc, argv);

  return base::LaunchUnitTests(
      argc, argv,
      base::Bind(&TestSuiteNoAtExit::Run, base::Unretained(&test_suite)));
}
