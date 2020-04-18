// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/resource_usage_reporter_type_converters.h"

#include <stddef.h>

#include "base/numerics/safe_conversions.h"

namespace mojo {

namespace {

content::mojom::ResourceTypeStatPtr StatToMojo(
    const blink::WebCache::ResourceTypeStat& obj) {
  content::mojom::ResourceTypeStatPtr stat =
      content::mojom::ResourceTypeStat::New();
  stat->count = obj.count;
  stat->size = obj.size;
  stat->decoded_size = obj.decoded_size;
  return stat;
}

blink::WebCache::ResourceTypeStat StatFromMojo(
    const content::mojom::ResourceTypeStat& obj) {
  blink::WebCache::ResourceTypeStat stat;
  stat.count = base::saturated_cast<size_t>(obj.count);
  stat.size = base::saturated_cast<size_t>(obj.size);
  stat.decoded_size = base::saturated_cast<size_t>(obj.decoded_size);
  return stat;
}

}  // namespace

// static
content::mojom::ResourceTypeStatsPtr
TypeConverter<content::mojom::ResourceTypeStatsPtr,
              blink::WebCache::ResourceTypeStats>::
    Convert(const blink::WebCache::ResourceTypeStats& obj) {
  content::mojom::ResourceTypeStatsPtr stats =
      content::mojom::ResourceTypeStats::New();
  stats->images = StatToMojo(obj.images);
  stats->css_style_sheets = StatToMojo(obj.css_style_sheets);
  stats->scripts = StatToMojo(obj.scripts);
  stats->xsl_style_sheets = StatToMojo(obj.xsl_style_sheets);
  stats->fonts = StatToMojo(obj.fonts);
  stats->other = StatToMojo(obj.other);
  return stats;
}

// static
blink::WebCache::ResourceTypeStats
TypeConverter<blink::WebCache::ResourceTypeStats,
              content::mojom::ResourceTypeStats>::
    Convert(const content::mojom::ResourceTypeStats& obj) {
  if (!obj.images || !obj.css_style_sheets || !obj.scripts ||
      !obj.xsl_style_sheets || !obj.fonts || !obj.other) {
    return {};
  }
  blink::WebCache::ResourceTypeStats stats;
  stats.images = StatFromMojo(*obj.images);
  stats.css_style_sheets = StatFromMojo(*obj.css_style_sheets);
  stats.scripts = StatFromMojo(*obj.scripts);
  stats.xsl_style_sheets = StatFromMojo(*obj.xsl_style_sheets);
  stats.fonts = StatFromMojo(*obj.fonts);
  stats.other = StatFromMojo(*obj.other);
  return stats;
}

}  // namespace mojo
