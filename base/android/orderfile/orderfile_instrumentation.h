// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_ORDERFILE_ORDERFILE_INSTRUMENTATION_H_
#define BASE_ANDROID_ORDERFILE_ORDERFILE_INSTRUMENTATION_H_

#include <cstdint>
#include <vector>

namespace base {
namespace android {
namespace orderfile {
constexpr int kPhases = 1;
constexpr size_t kStartOfTextForTesting = 1000;
constexpr size_t kEndOfTextForTesting = kStartOfTextForTesting + 1000 * 1000;

// Stop recording.
void Disable();

// CHECK()s that the offsets are correctly set up.
void SanityChecks();

// Switches to the next recording phase. If called from the last phase, dumps
// the data to disk, and returns |true|. |pid| is the current process pid, and
// |start_ns_since_epoch| the process start timestamp.
bool SwitchToNextPhaseOrDump(int pid, uint64_t start_ns_since_epoch);

// Starts a thread to dump instrumentation after a delay.
void StartDelayedDump();

// Record an |address|, if recording is enabled. Only for testing.
void RecordAddressForTesting(size_t address);

// Resets the state. Only for testing.
void ResetForTesting();

// Returns an ordered list of reached offsets. Only for testing.
std::vector<size_t> GetOrderedOffsetsForTesting();
}  // namespace orderfile
}  // namespace android
}  // namespace base

#endif  // BASE_ANDROID_ORDERFILE_ORDERFILE_INSTRUMENTATION_H_
