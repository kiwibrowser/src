// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/hit_test_cache.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/histogram.h"

namespace blink {

bool HitTestCache::LookupCachedResult(HitTestResult& hit_result,
                                      uint64_t dom_tree_version) {
  bool result = false;
  HitHistogramMetric metric = HitHistogramMetric::MISS;
  if (hit_result.GetHitTestRequest().AvoidCache()) {
    metric = HitHistogramMetric::MISS_EXPLICIT_AVOID;
    // For now we don't support rect based hit results.
  } else if (dom_tree_version == dom_tree_version_ &&
             !hit_result.GetHitTestLocation().IsRectBasedTest()) {
    for (const auto& cached_item : items_) {
      if (cached_item.GetHitTestLocation().Point() ==
          hit_result.GetHitTestLocation().Point()) {
        if (hit_result.GetHitTestRequest().EqualForCacheability(
                cached_item.GetHitTestRequest())) {
          metric = HitHistogramMetric::HIT_EXACT_MATCH;
          result = true;
          hit_result = cached_item;
          break;
        }
        metric = HitHistogramMetric::MISS_VALIDITY_RECT_MATCHES;
      }
    }
  }
  DEFINE_STATIC_LOCAL(
      EnumerationHistogram, hit_test_histogram,
      ("Event.HitTest",
       static_cast<int32_t>(HitHistogramMetric::MAX_HIT_METRIC)));
  hit_test_histogram.Count(static_cast<int32_t>(metric));
  return result;
}

void HitTestCache::AddCachedResult(const HitTestResult& result,
                                   uint64_t dom_tree_version) {
  if (!result.IsCacheable())
    return;

  // If the result was a hit test on an LayoutEmbeddedContent and the request
  // allowed querying of the layout part; then the part hasn't been loaded yet.
  if (result.IsOverEmbeddedContentView() &&
      result.GetHitTestRequest().AllowsChildFrameContent())
    return;

  // For now don't support rect based or list based requests.
  if (result.GetHitTestLocation().IsRectBasedTest() ||
      result.GetHitTestRequest().ListBased())
    return;
  if (dom_tree_version != dom_tree_version_)
    Clear();
  if (items_.size() < HIT_TEST_CACHE_SIZE)
    items_.resize(update_index_ + 1);

  items_.at(update_index_).CacheValues(result);
  dom_tree_version_ = dom_tree_version;

  update_index_++;
  if (update_index_ >= HIT_TEST_CACHE_SIZE)
    update_index_ = 0;
}

void HitTestCache::Clear() {
  update_index_ = 0;
  items_.clear();
}

void HitTestCache::Trace(blink::Visitor* visitor) {
  visitor->Trace(items_);
}

}  // namespace blink
