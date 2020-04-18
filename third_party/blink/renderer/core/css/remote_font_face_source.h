// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_REMOTE_FONT_FACE_SOURCE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_REMOTE_FONT_FACE_SOURCE_H_

#include "third_party/blink/renderer/core/css/css_font_face_source.h"
#include "third_party/blink/renderer/core/loader/resource/font_resource.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class CSSFontFace;
class FontSelector;
class FontCustomPlatformData;

class RemoteFontFaceSource final : public CSSFontFaceSource,
                                   public FontResourceClient {
  USING_PRE_FINALIZER(RemoteFontFaceSource, Dispose);
  USING_GARBAGE_COLLECTED_MIXIN(RemoteFontFaceSource);

 public:
  enum Phase { kNoLimitExceeded, kShortLimitExceeded, kLongLimitExceeded };
  // Periods of the Font Display Timeline.
  // https://drafts.csswg.org/css-fonts-4/#font-display-timeline
  enum DisplayPeriod { kBlockPeriod, kSwapPeriod, kFailurePeriod };

  RemoteFontFaceSource(CSSFontFace*, FontSelector*, FontDisplay);
  ~RemoteFontFaceSource() override;
  void Dispose();

  bool IsLoading() const override;
  bool IsLoaded() const override;
  bool IsValid() const override;

  void BeginLoadIfNeeded() override;
  void SetDisplay(FontDisplay) override;

  void NotifyFinished(Resource*) override;
  void FontLoadShortLimitExceeded(FontResource*) override;
  void FontLoadLongLimitExceeded(FontResource*) override;
  String DebugName() const override { return "RemoteFontFaceSource"; }

  bool IsInBlockPeriod() const override { return period_ == kBlockPeriod; }
  bool IsInFailurePeriod() const override { return period_ == kFailurePeriod; }

  // For UMA reporting
  bool HadBlankText() override { return histograms_.HadBlankText(); }
  void PaintRequested() { histograms_.FallbackFontPainted(period_); }

  void Trace(blink::Visitor*) override;

 protected:
  scoped_refptr<SimpleFontData> CreateFontData(
      const FontDescription&,
      const FontSelectionCapabilities&) override;
  scoped_refptr<SimpleFontData> CreateLoadingFallbackFontData(
      const FontDescription&);

 private:
  class FontLoadHistograms {
    DISALLOW_NEW();

   public:
    // Should not change following order in CacheHitMetrics to be used for
    // metrics values.
    enum CacheHitMetrics {
      kMiss,
      kDiskHit,
      kDataUrl,
      kMemoryHit,
      kCacheHitEnumMax
    };
    enum DataSource {
      kFromUnknown,
      kFromDataURL,
      kFromMemoryCache,
      kFromDiskCache,
      kFromNetwork
    };

    FontLoadHistograms()
        : load_start_time_(0),
          blank_paint_time_(0),
          is_long_limit_exceeded_(false),
          data_source_(kFromUnknown) {}
    void LoadStarted();
    void FallbackFontPainted(DisplayPeriod);
    void LongLimitExceeded();
    void RecordFallbackTime();
    void RecordRemoteFont(const FontResource*);
    bool HadBlankText() { return blank_paint_time_; }
    DataSource GetDataSource() { return data_source_; }
    void MaySetDataSource(DataSource);

    static DataSource DataSourceForLoadFinish(const FontResource* font) {
      if (font->Url().ProtocolIsData())
        return kFromDataURL;
      return font->GetResponse().WasCached() ? kFromDiskCache : kFromNetwork;
    }

   private:
    void RecordLoadTimeHistogram(const FontResource*, int duration);
    CacheHitMetrics DataSourceMetricsValue();
    double load_start_time_;
    double blank_paint_time_;
    bool is_long_limit_exceeded_;
    DataSource data_source_;
  };

  void UpdatePeriod();
  bool ShouldTriggerWebFontsIntervention();
  bool IsLowPriorityLoadingAllowedForRemoteFont() const override;

  // Our owning font face.
  Member<CSSFontFace> face_;
  Member<FontSelector> font_selector_;

  // |nullptr| if font is not loaded or failed to decode.
  scoped_refptr<FontCustomPlatformData> custom_font_data_;

  FontDisplay display_;
  Phase phase_;
  DisplayPeriod period_;
  FontLoadHistograms histograms_;
  bool is_intervention_triggered_;
};

}  // namespace blink

#endif
