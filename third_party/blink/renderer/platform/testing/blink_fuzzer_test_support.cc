// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/testing/blink_fuzzer_test_support.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "content/public/test/blink_test_environment.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"

namespace blink {

BlinkFuzzerTestSupport::BlinkFuzzerTestSupport()
    : BlinkFuzzerTestSupport(0, nullptr) {}

BlinkFuzzerTestSupport::BlinkFuzzerTestSupport(int argc, char** argv) {
  // Note: we don't tear anything down here after an iteration of the fuzzer
  // is complete, this is for efficiency. We rerun the fuzzer with the same
  // environment as the previous iteration.
  base::AtExitManager at_exit;

  CHECK(base::i18n::InitializeICU());

  base::CommandLine::Init(argc, argv);

  content::SetUpBlinkTestEnvironment();

  blink::SchemeRegistry::Initialize();
}

BlinkFuzzerTestSupport::~BlinkFuzzerTestSupport() = default;

}  // namespace blink
