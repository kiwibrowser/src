// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "media/base/media_switches.h"
#include "services/audio/service.h"
#include "services/audio/standalone_unittest_catalog_source.h"
#include "services/audio/test/service_lifetime_test_template.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"

namespace audio {

class StandaloneAudioServiceTest : public service_manager::test::ServiceTest {
 public:
  StandaloneAudioServiceTest() : ServiceTest("audio_unittests") {}

  ~StandaloneAudioServiceTest() override {}

 protected:
  // service_manager::test::ServiceTest:
  std::unique_ptr<base::Value> CreateCustomTestCatalog() override {
    return audio::CreateStandaloneUnittestCatalog();
  }

  void SetUp() override {
    ServiceTest::SetUp();
    base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
    cmd_line->AppendSwitchASCII(switches::kAudioServiceQuitTimeoutMs,
                                base::UintToString(10));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(StandaloneAudioServiceTest);
};

INSTANTIATE_TYPED_TEST_CASE_P(StandaloneAudioService,
                              ServiceLifetimeTestTemplate,
                              StandaloneAudioServiceTest);
}  // namespace audio
