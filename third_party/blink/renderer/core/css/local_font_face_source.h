// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_LOCAL_FONT_FACE_SOURCE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_LOCAL_FONT_FACE_SOURCE_H_

#include "third_party/blink/renderer/core/css/css_font_face_source.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class LocalFontFaceSource final : public CSSFontFaceSource {
 public:
  LocalFontFaceSource(const String& font_name) : font_name_(font_name) {}
  bool IsLocal() const override { return true; }
  bool IsLocalFontAvailable(const FontDescription&) override;

 private:
  scoped_refptr<SimpleFontData> CreateFontData(
      const FontDescription&,
      const FontSelectionCapabilities&) override;

  class LocalFontHistograms {
    DISALLOW_NEW();

   public:
    LocalFontHistograms() : reported_(false) {}
    void Record(bool load_success);

   private:
    bool reported_;
  };

  AtomicString font_name_;
  LocalFontHistograms histograms_;
};

}  // namespace blink

#endif
