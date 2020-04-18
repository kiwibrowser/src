// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_SWITCHES_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_SWITCHES_H_

namespace heap_profiling {

extern const char kMemlog[];
extern const char kMemlogKeepSmallAllocations[];
extern const char kMemlogModeAll[];
extern const char kMemlogModeAllRenderers[];
extern const char kMemlogModeBrowser[];
extern const char kMemlogModeGpu[];
extern const char kMemlogModeManual[];
extern const char kMemlogModeMinimal[];
extern const char kMemlogModeRendererSampling[];
extern const char kMemlogSampling[];
extern const char kMemlogSamplingRate[];
extern const char kMemlogStackMode[];
extern const char kMemlogStackModeMixed[];
extern const char kMemlogStackModeNative[];
extern const char kMemlogStackModeNativeWithThreadNames[];
extern const char kMemlogStackModePseudo[];

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_SWITCHES_H_
