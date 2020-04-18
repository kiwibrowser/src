/*
 * Copyright (c) 2011 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/platform/fonts/font_cache.h"

#include "third_party/blink/renderer/platform/font_family_names.h"
#include "third_party/blink/renderer/platform/fonts/font_description.h"
#include "third_party/blink/renderer/platform/fonts/font_face_creation_params.h"
#include "third_party/blink/renderer/platform/fonts/simple_font_data.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "third_party/skia/include/ports/SkFontMgr.h"

namespace blink {

static AtomicString DefaultFontFamily(sk_sp<SkFontMgr> font_manager) {
  // Pass nullptr to get the default typeface. The default typeface in Android
  // is "sans-serif" if exists, or the first entry in fonts.xml.
  sk_sp<SkTypeface> typeface(
      font_manager->legacyMakeTypeface(nullptr, SkFontStyle()));
  if (typeface) {
    SkString family_name;
    typeface->getFamilyName(&family_name);
    if (family_name.size())
      return ToAtomicString(family_name);
  }

  // Some devices do not return the default typeface. There's not much we can
  // do here, use "Arial", the value LayoutTheme uses for CSS system font
  // keywords such as "menu".
  NOTREACHED();
  return FontFamilyNames::Arial;
}

static AtomicString DefaultFontFamily() {
  if (sk_sp<SkFontMgr> font_manager = FontCache::GetFontCache()->FontManager())
    return DefaultFontFamily(font_manager);
  return DefaultFontFamily(SkFontMgr::RefDefault());
}

// static
const AtomicString& FontCache::SystemFontFamily() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(AtomicString, system_font_family,
                                  (DefaultFontFamily()));
  return system_font_family;
}

// static
void FontCache::SetSystemFontFamily(const AtomicString&) {}

scoped_refptr<SimpleFontData> FontCache::PlatformFallbackFontForCharacter(
    const FontDescription& font_description,
    UChar32 c,
    const SimpleFontData*,
    FontFallbackPriority fallback_priority) {
  sk_sp<SkFontMgr> fm(SkFontMgr::RefDefault());
  AtomicString family_name = GetFamilyNameForCharacter(
      fm.get(), c, font_description, fallback_priority);
  if (family_name.IsEmpty())
    return GetLastResortFallbackFont(font_description, kDoNotRetain);
  return FontDataFromFontPlatformData(
      GetFontPlatformData(font_description,
                          FontFaceCreationParams(family_name)),
      kDoNotRetain);
}

// static
AtomicString FontCache::GetGenericFamilyNameForScript(
    const AtomicString& family_name,
    const FontDescription& font_description) {
  // If monospace, do not apply CJK hack to find i18n fonts, because
  // i18n fonts are likely not monospace. Monospace is mostly used
  // for code, but when i18n characters appear in monospace, system
  // fallback can still render the characters.
  if (family_name == FontFamilyNames::webkit_monospace)
    return family_name;

  // The CJK hack below should be removed, at latest when we have
  // serif and sans-serif versions of CJK fonts. Until then, limit it
  // to only when the content locale is available. crbug.com/652146
  const LayoutLocale* content_locale = font_description.Locale();
  if (!content_locale)
    return family_name;

  // This is a hack to use the preferred font for CJK scripts.
  // TODO(kojii): This logic disregards either generic family name
  // or locale. We need an API that honors both to find appropriate
  // fonts. crbug.com/642340
  UChar32 exampler_char;
  switch (content_locale->GetScript()) {
    case USCRIPT_SIMPLIFIED_HAN:
    case USCRIPT_TRADITIONAL_HAN:
    case USCRIPT_KATAKANA_OR_HIRAGANA:
      exampler_char = 0x4E00;  // A common character in Japanese and Chinese.
      break;
    case USCRIPT_HANGUL:
      exampler_char = 0xAC00;
      break;
    default:
      // For other scripts, use the default generic family mapping logic.
      return family_name;
  }

  sk_sp<SkFontMgr> fm(SkFontMgr::RefDefault());
  return GetFamilyNameForCharacter(fm.get(), exampler_char, font_description,
                                   FontFallbackPriority::kText);
}

}  // namespace blink
