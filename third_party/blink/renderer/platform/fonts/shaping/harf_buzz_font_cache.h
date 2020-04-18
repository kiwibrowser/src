// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SHAPING_HARF_BUZZ_FONT_CACHE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SHAPING_HARF_BUZZ_FONT_CACHE_H_

#include <memory>
#include "third_party/blink/renderer/platform/fonts/font_metrics.h"
#include "third_party/blink/renderer/platform/fonts/opentype/open_type_vertical_data.h"
#include "third_party/blink/renderer/platform/fonts/shaping/harf_buzz_face.h"
#include "third_party/blink/renderer/platform/fonts/unicode_range_set.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

struct hb_font_t;
struct hb_face_t;

namespace blink {

struct HbFontDeleter {
  void operator()(hb_font_t* font);
};

using HbFontUniquePtr = std::unique_ptr<hb_font_t, HbFontDeleter>;

struct HbFaceDeleter {
  void operator()(hb_face_t* face);
};

using HbFaceUniquePtr = std::unique_ptr<hb_face_t, HbFaceDeleter>;

const unsigned kInvalidFallbackMetricsValue = static_cast<unsigned>(-1);

// struct to carry user-pointer data for hb_font_t callback
// functions/operations, that require information related to a font scaled to a
// particular size.
struct HarfBuzzFontData {
  USING_FAST_MALLOC(HarfBuzzFontData);
  WTF_MAKE_NONCOPYABLE(HarfBuzzFontData);

 public:
  HarfBuzzFontData()
      : paint_(),
        space_in_gpos_(SpaceGlyphInOpenTypeTables::Unknown),
        space_in_gsub_(SpaceGlyphInOpenTypeTables::Unknown),
        vertical_data_(nullptr),
        range_set_(nullptr) {}

  // The vertical origin and vertical advance functions in HarfBuzzFace require
  // the ascent and height metrics as fallback in case no specific vertical
  // layout information is found from the font.
  void UpdateFallbackMetricsAndScale(
      const FontPlatformData& platform_data,
      const SkPaint& paint,
      HarfBuzzFace::VerticalLayoutCallbacks vertical_layout) {
    float ascent = 0;
    float descent = 0;
    unsigned dummy_ascent_inflation = 0;
    unsigned dummy_descent_inflation = 0;

    paint_ = paint;

    if (UNLIKELY(vertical_layout == HarfBuzzFace::PrepareForVerticalLayout)) {
      FontMetrics::AscentDescentWithHacks(
          ascent, descent, dummy_ascent_inflation, dummy_descent_inflation,
          platform_data, paint);
      ascent_fallback_ = ascent;
      // Simulate the rounding that FontMetrics does so far for returning the
      // integer Height()
      height_fallback_ = lroundf(ascent) + lroundf(descent);

      int units_per_em =
          platform_data.GetHarfBuzzFace()->UnitsPerEmFromHeadTable();
      if (!units_per_em) {
        DLOG(ERROR)
            << "Units per EM is 0 for font used in vertical writing mode.";
      }
      size_per_unit_ = platform_data.size() / (units_per_em ? units_per_em : 1);
    } else {
      ascent_fallback_ = kInvalidFallbackMetricsValue;
      height_fallback_ = kInvalidFallbackMetricsValue;
      size_per_unit_ = kInvalidFallbackMetricsValue;
    }
  }

  scoped_refptr<OpenTypeVerticalData> VerticalData() {
    if (!vertical_data_) {
      DCHECK_NE(ascent_fallback_, kInvalidFallbackMetricsValue);
      DCHECK_NE(height_fallback_, kInvalidFallbackMetricsValue);
      DCHECK_NE(size_per_unit_, kInvalidFallbackMetricsValue);

      vertical_data_ =
          OpenTypeVerticalData::CreateUnscaled(paint_.refTypeface());
    }
    vertical_data_->SetScaleAndFallbackMetrics(size_per_unit_, ascent_fallback_,
                                               height_fallback_);
    return vertical_data_;
  }

  SkPaint paint_;

  // Capture these scaled fallback metrics from FontPlatformData so that a
  // OpenTypeVerticalData object can be constructed from them when needed.
  float size_per_unit_;
  float ascent_fallback_;
  float height_fallback_;

  enum class SpaceGlyphInOpenTypeTables { Unknown, Present, NotPresent };

  SpaceGlyphInOpenTypeTables space_in_gpos_;
  SpaceGlyphInOpenTypeTables space_in_gsub_;

  scoped_refptr<OpenTypeVerticalData> vertical_data_;
  scoped_refptr<UnicodeRangeSet> range_set_;
};

// Though we have FontCache class, which provides the cache mechanism for
// WebKit's font objects, we also need additional caching layer for HarfBuzz to
// reduce the number of hb_font_t objects created. Without it, we would create
// an hb_font_t object for every FontPlatformData object. But insted, we only
// need one for each unique SkTypeface.
// FIXME, crbug.com/609099: We should fix the FontCache to only keep one
// FontPlatformData object independent of size, then consider using this here.
class HbFontCacheEntry : public RefCounted<HbFontCacheEntry> {
 public:
  static scoped_refptr<HbFontCacheEntry> Create(hb_font_t* hb_font) {
    DCHECK(hb_font);
    return base::AdoptRef(new HbFontCacheEntry(hb_font));
  }

  hb_font_t* HbFont() { return hb_font_.get(); }
  HarfBuzzFontData* HbFontData() { return hb_font_data_.get(); }

 private:
  explicit HbFontCacheEntry(hb_font_t* font)
      : hb_font_(HbFontUniquePtr(font)),
        hb_font_data_(std::make_unique<HarfBuzzFontData>()){};

  HbFontUniquePtr hb_font_;
  std::unique_ptr<HarfBuzzFontData> hb_font_data_;
};

typedef HashMap<uint64_t,
                scoped_refptr<HbFontCacheEntry>,
                WTF::IntHash<uint64_t>,
                WTF::UnsignedWithZeroKeyHashTraits<uint64_t>>
    HarfBuzzFontCache;

}  // namespace blink

#endif
