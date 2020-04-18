// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/opentype/open_type_caps_support.h"

namespace blink {

OpenTypeCapsSupport::OpenTypeCapsSupport()
    : harf_buzz_face_(nullptr),
      requested_caps_(FontDescription::kCapsNormal),
      font_support_(FontSupport::kFull),
      caps_synthesis_(CapsSynthesis::kNone) {}

OpenTypeCapsSupport::OpenTypeCapsSupport(
    const HarfBuzzFace* harf_buzz_face,
    FontDescription::FontVariantCaps requested_caps,
    hb_script_t script)
    : harf_buzz_face_(harf_buzz_face),
      requested_caps_(requested_caps),
      font_support_(FontSupport::kFull),
      caps_synthesis_(CapsSynthesis::kNone) {
  if (requested_caps != FontDescription::kCapsNormal)
    DetermineFontSupport(script);
}

FontDescription::FontVariantCaps OpenTypeCapsSupport::FontFeatureToUse(
    SmallCapsIterator::SmallCapsBehavior source_text_case) {
  if (font_support_ == FontSupport::kFull)
    return requested_caps_;

  if (font_support_ == FontSupport::kFallback) {
    if (requested_caps_ == FontDescription::FontVariantCaps::kAllPetiteCaps)
      return FontDescription::FontVariantCaps::kAllSmallCaps;

    if (requested_caps_ == FontDescription::FontVariantCaps::kPetiteCaps ||
        (requested_caps_ == FontDescription::FontVariantCaps::kUnicase &&
         source_text_case == SmallCapsIterator::kSmallCapsSameCase))
      return FontDescription::FontVariantCaps::kSmallCaps;
  }

  return FontDescription::FontVariantCaps::kCapsNormal;
}

bool OpenTypeCapsSupport::NeedsRunCaseSplitting() {
  // Lack of titling case support is ignored, titling case is not synthesized.
  return font_support_ != FontSupport::kFull &&
         requested_caps_ != FontDescription::kTitlingCaps;
}

bool OpenTypeCapsSupport::NeedsSyntheticFont(
    SmallCapsIterator::SmallCapsBehavior run_case) {
  if (font_support_ == FontSupport::kFull)
    return false;

  if (requested_caps_ == FontDescription::kTitlingCaps)
    return false;

  if (font_support_ == FontSupport::kNone) {
    if (run_case == SmallCapsIterator::kSmallCapsUppercaseNeeded &&
        (caps_synthesis_ == CapsSynthesis::kLowerToSmallCaps ||
         caps_synthesis_ == CapsSynthesis::kBothToSmallCaps))
      return true;

    if (run_case == SmallCapsIterator::kSmallCapsSameCase &&
        (caps_synthesis_ == CapsSynthesis::kUpperToSmallCaps ||
         caps_synthesis_ == CapsSynthesis::kBothToSmallCaps)) {
      return true;
    }
  }

  return false;
}

CaseMapIntend OpenTypeCapsSupport::NeedsCaseChange(
    SmallCapsIterator::SmallCapsBehavior run_case) {
  CaseMapIntend case_map_intend = CaseMapIntend::kKeepSameCase;

  if (font_support_ == FontSupport::kFull)
    return case_map_intend;

  switch (run_case) {
    case SmallCapsIterator::kSmallCapsSameCase:
      case_map_intend =
          font_support_ == FontSupport::kFallback &&
                  (caps_synthesis_ == CapsSynthesis::kBothToSmallCaps ||
                   caps_synthesis_ == CapsSynthesis::kUpperToSmallCaps)
              ? CaseMapIntend::kLowerCase
              : CaseMapIntend::kKeepSameCase;
      break;
    case SmallCapsIterator::kSmallCapsUppercaseNeeded:
      case_map_intend =
          font_support_ != FontSupport::kFallback &&
                  (caps_synthesis_ == CapsSynthesis::kLowerToSmallCaps ||
                   caps_synthesis_ == CapsSynthesis::kBothToSmallCaps)
              ? CaseMapIntend::kUpperCase
              : CaseMapIntend::kKeepSameCase;
      break;
    default:
      break;
  }
  return case_map_intend;
}

void OpenTypeCapsSupport::DetermineFontSupport(hb_script_t script) {
  switch (requested_caps_) {
    case FontDescription::kSmallCaps:
      if (!SupportsOpenTypeFeature(script, HB_TAG('s', 'm', 'c', 'p'))) {
        font_support_ = FontSupport::kNone;
        caps_synthesis_ = CapsSynthesis::kLowerToSmallCaps;
      }
      break;
    case FontDescription::kAllSmallCaps:
      if (!(SupportsOpenTypeFeature(script, HB_TAG('s', 'm', 'c', 'p')) &&
            SupportsOpenTypeFeature(script, HB_TAG('c', '2', 's', 'c')))) {
        font_support_ = FontSupport::kNone;
        caps_synthesis_ = CapsSynthesis::kBothToSmallCaps;
      }
      break;
    case FontDescription::kPetiteCaps:
      if (!SupportsOpenTypeFeature(script, HB_TAG('p', 'c', 'a', 'p'))) {
        if (SupportsOpenTypeFeature(script, HB_TAG('s', 'm', 'c', 'p'))) {
          font_support_ = FontSupport::kFallback;
        } else {
          font_support_ = FontSupport::kNone;
          caps_synthesis_ = CapsSynthesis::kLowerToSmallCaps;
        }
      }
      break;
    case FontDescription::kAllPetiteCaps:
      if (!(SupportsOpenTypeFeature(script, HB_TAG('p', 'c', 'a', 'p')) &&
            SupportsOpenTypeFeature(script, HB_TAG('c', '2', 'p', 'c')))) {
        if (SupportsOpenTypeFeature(script, HB_TAG('s', 'm', 'c', 'p')) &&
            SupportsOpenTypeFeature(script, HB_TAG('c', '2', 's', 'c'))) {
          font_support_ = FontSupport::kFallback;
        } else {
          font_support_ = FontSupport::kNone;
          caps_synthesis_ = CapsSynthesis::kBothToSmallCaps;
        }
      }
      break;
    case FontDescription::kUnicase:
      if (!SupportsOpenTypeFeature(script, HB_TAG('u', 'n', 'i', 'c'))) {
        caps_synthesis_ = CapsSynthesis::kUpperToSmallCaps;
        if (SupportsOpenTypeFeature(script, HB_TAG('s', 'm', 'c', 'p'))) {
          font_support_ = FontSupport::kFallback;
        } else {
          font_support_ = FontSupport::kNone;
        }
      }
      break;
    case FontDescription::kTitlingCaps:
      if (!SupportsOpenTypeFeature(script, HB_TAG('t', 'i', 't', 'l'))) {
        font_support_ = FontSupport::kNone;
      }
      break;
    default:
      NOTREACHED();
  }
}

}  // namespace blink
