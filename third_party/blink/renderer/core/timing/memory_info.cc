/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/timing/memory_info.h"

#include <limits>

#include "base/macros.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"
#include "third_party/blink/renderer/platform/wtf/thread_specific.h"
#include "third_party/blink/renderer/platform/wtf/time.h"
#include "v8/include/v8.h"

namespace blink {

static const double kTwentyMinutesInSeconds = 20 * 60;

static void GetHeapSize(HeapInfo& info) {
  v8::HeapStatistics heap_statistics;
  v8::Isolate::GetCurrent()->GetHeapStatistics(&heap_statistics);
  info.used_js_heap_size = heap_statistics.used_heap_size();
  info.total_js_heap_size = heap_statistics.total_physical_size();
  info.js_heap_size_limit = heap_statistics.heap_size_limit();
}

class HeapSizeCache {
  USING_FAST_MALLOC(HeapSizeCache);

 public:
  HeapSizeCache()
      : last_update_time_(CurrentTimeTicksInSeconds() -
                          kTwentyMinutesInSeconds) {}

  void GetCachedHeapSize(HeapInfo& info) {
    MaybeUpdate();
    info = info_;
  }

  static HeapSizeCache& ForCurrentThread() {
    DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<HeapSizeCache>,
                                    heap_size_cache, ());
    return *heap_size_cache;
  }

 private:
  void MaybeUpdate() {
    // We rate-limit queries to once every twenty minutes to make it more
    // difficult for attackers to compare memory usage before and after some
    // event.
    double now = CurrentTimeTicksInSeconds();
    if (now - last_update_time_ >= kTwentyMinutesInSeconds) {
      Update();
      last_update_time_ = now;
    }
  }

  void Update() {
    GetHeapSize(info_);
    info_.used_js_heap_size = QuantizeMemorySize(info_.used_js_heap_size);
    info_.total_js_heap_size = QuantizeMemorySize(info_.total_js_heap_size);
    info_.js_heap_size_limit = QuantizeMemorySize(info_.js_heap_size_limit);
  }

  double last_update_time_;

  HeapInfo info_;
  DISALLOW_COPY_AND_ASSIGN(HeapSizeCache);
};

// We quantize the sizes to make it more difficult for an attacker to see
// precise impact of operations on memory. The values are used for performance
// tuning, and hence don't need to be as refined when the value is large, so we
// threshold at a list of exponentially separated buckets.
size_t QuantizeMemorySize(size_t size) {
  const int kNumberOfBuckets = 100;
  DEFINE_STATIC_LOCAL(Vector<size_t>, bucket_size_list, ());

  if (bucket_size_list.IsEmpty()) {
    bucket_size_list.resize(kNumberOfBuckets);

    float size_of_next_bucket =
        10000000.0;  // First bucket size is roughly 10M.
    const float kLargestBucketSize = 4000000000.0;  // Roughly 4GB.
    // We scale with the Nth root of the ratio, so that we use all the bucktes.
    const float scaling_factor =
        exp(log(kLargestBucketSize / size_of_next_bucket) / kNumberOfBuckets);

    size_t next_power_of_ten = static_cast<size_t>(
        pow(10, floor(log10(size_of_next_bucket)) + 1) + 0.5);
    size_t granularity =
        next_power_of_ten / 1000;  // We want 3 signficant digits.

    for (int i = 0; i < kNumberOfBuckets; ++i) {
      size_t current_bucket_size = static_cast<size_t>(size_of_next_bucket);
      bucket_size_list[i] =
          current_bucket_size - (current_bucket_size % granularity);

      size_of_next_bucket *= scaling_factor;
      if (size_of_next_bucket >= next_power_of_ten) {
        if (std::numeric_limits<size_t>::max() / 10 <= next_power_of_ten) {
          next_power_of_ten = std::numeric_limits<size_t>::max();
        } else {
          next_power_of_ten *= 10;
          granularity *= 10;
        }
      }

      // Watch out for overflow, if the range is too large for size_t.
      if (i > 0 && bucket_size_list[i] < bucket_size_list[i - 1])
        bucket_size_list[i] = std::numeric_limits<size_t>::max();
    }
  }

  for (int i = 0; i < kNumberOfBuckets; ++i) {
    if (size <= bucket_size_list[i])
      return bucket_size_list[i];
  }

  return bucket_size_list[kNumberOfBuckets - 1];
}

MemoryInfo::MemoryInfo() {
  if (RuntimeEnabledFeatures::PreciseMemoryInfoEnabled())
    GetHeapSize(info_);
  else
    HeapSizeCache::ForCurrentThread().GetCachedHeapSize(info_);
}

}  // namespace blink
