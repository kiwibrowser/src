// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/font_selector.h"

#include "build/build_config.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/fonts/font_description.h"
#include "third_party/blink/renderer/platform/fonts/generic_font_family_settings.h"

namespace blink {

AtomicString FontSelector::FamilyNameFromSettings(
    const GenericFontFamilySettings& settings,
    const FontDescription& font_description,
    const AtomicString& generic_family_name) {
#if defined(OS_ANDROID)
  if (font_description.GenericFamily() == FontDescription::kStandardFamily) {
    return FontCache::GetGenericFamilyNameForScript(
        FontFamilyNames::webkit_standard, font_description);
  }

  if (generic_family_name.StartsWith("-webkit-")) {
    return FontCache::GetGenericFamilyNameForScript(generic_family_name,
                                                    font_description);
  }
#else
  UScriptCode script = font_description.GetScript();
  if (font_description.GenericFamily() == FontDescription::kStandardFamily)
    return settings.Standard(script);
  if (generic_family_name == FontFamilyNames::webkit_serif)
    return settings.Serif(script);
  if (generic_family_name == FontFamilyNames::webkit_sans_serif)
    return settings.SansSerif(script);
  if (generic_family_name == FontFamilyNames::webkit_cursive)
    return settings.Cursive(script);
  if (generic_family_name == FontFamilyNames::webkit_fantasy)
    return settings.Fantasy(script);
  if (generic_family_name == FontFamilyNames::webkit_monospace)
    return settings.Fixed(script);
  if (generic_family_name == FontFamilyNames::webkit_pictograph)
    return settings.Pictograph(script);
  if (generic_family_name == FontFamilyNames::webkit_standard)
    return settings.Standard(script);
#endif
  return g_empty_atom;
}

}  // namespace blink
