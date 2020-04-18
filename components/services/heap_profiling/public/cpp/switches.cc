// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/public/cpp/switches.h"

namespace heap_profiling {

const char kMemlog[] = "memlog";
const char kMemlogKeepSmallAllocations[] = "memlog-keep-small-allocations";
const char kMemlogModeAll[] = "all";
const char kMemlogModeAllRenderers[] = "all-renderers";
const char kMemlogModeBrowser[] = "browser";
const char kMemlogModeGpu[] = "gpu";
const char kMemlogModeManual[] = "manual";
const char kMemlogModeMinimal[] = "minimal";
const char kMemlogModeRendererSampling[] = "renderer-sampling";
const char kMemlogSampling[] = "memlog-sampling";
const char kMemlogSamplingRate[] = "memlog-sampling-rate";
const char kMemlogStackMode[] = "memlog-stack-mode";
const char kMemlogStackModeMixed[] = "mixed";
const char kMemlogStackModeNative[] = "native";
const char kMemlogStackModeNativeWithThreadNames[] = "native-with-thread-names";
const char kMemlogStackModePseudo[] = "pseudo";

}  // namespace heap_profiling
