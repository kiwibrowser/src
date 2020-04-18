// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_OPENTYPE_OPEN_TYPE_CAPS_SUPPORT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_OPENTYPE_OPEN_TYPE_CAPS_SUPPORT_H_

#include "third_party/blink/renderer/platform/fonts/font_description.h"
#include "third_party/blink/renderer/platform/fonts/opentype/open_type_caps_support.h"
#include "third_party/blink/renderer/platform/fonts/shaping/case_mapping_harf_buzz_buffer_filler.h"
#include "third_party/blink/renderer/platform/fonts/shaping/harf_buzz_face.h"
#include "third_party/blink/renderer/platform/fonts/small_caps_iterator.h"

#include <hb.h>

namespace blink {

class PLATFORM_EXPORT OpenTypeCapsSupport {
 public:
  OpenTypeCapsSupport();
  OpenTypeCapsSupport(const HarfBuzzFace*,
                      FontDescription::FontVariantCaps requested_caps,
                      hb_script_t);

  bool NeedsRunCaseSplitting();
  bool NeedsSyntheticFont(SmallCapsIterator::SmallCapsBehavior run_case);
  FontDescription::FontVariantCaps FontFeatureToUse(
      SmallCapsIterator::SmallCapsBehavior run_case);
  CaseMapIntend NeedsCaseChange(SmallCapsIterator::SmallCapsBehavior run_case);

 private:
  void DetermineFontSupport(hb_script_t);
  bool SupportsOpenTypeFeature(hb_script_t, uint32_t tag) const;

  const HarfBuzzFace* harf_buzz_face_;
  FontDescription::FontVariantCaps requested_caps_;

  enum class FontSupport {
    kFull,
    kFallback,  // Fall back to 'smcp' or 'smcp' + 'c2sc'
    kNone
  };

  enum class CapsSynthesis {
    kNone,
    kLowerToSmallCaps,
    kUpperToSmallCaps,
    kBothToSmallCaps
  };

  FontSupport font_support_;
  CapsSynthesis caps_synthesis_;
};

};  // namespace blink

#endif
