// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/web_font_typeface_factory.h"

#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/fonts/opentype/font_format_check.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkTypeface.h"
#if defined(OS_WIN) || defined(OS_MACOSX)
#include "third_party/skia/include/ports/SkFontMgr_empty.h"
#endif
#if defined(OS_MACOSX)
#include "third_party/blink/renderer/platform/fonts/mac/core_text_variations_support.h"
#endif

namespace blink {

bool WebFontTypefaceFactory::CreateTypeface(sk_sp<SkData> sk_data,
                                            sk_sp<SkTypeface>& typeface) {
  CHECK(!typeface);

  FontFormatCheck format_check(sk_data);

  std::unique_ptr<SkStreamAsset> stream(new SkMemoryStream(sk_data));

  if (!format_check.IsVariableFont() && !format_check.IsCbdtCblcColorFont() &&
      !format_check.IsCff2OutlineFont() && !format_check.IsSbixColorFont()) {
    typeface = DefaultFontManager()->makeFromStream(std::move(stream));
    if (typeface) {
      ReportWebFontInstantiationResult(kSuccessConventionalWebFont);
      return true;
    }
    // Not UMA reporting general decoding errors as these are already recorded
    // as kPackageFormatUnknown in FontResource.cpp.
    return false;
  }

  if (format_check.IsCbdtCblcColorFont()) {
    typeface = FreeTypeFontManager()->makeFromStream(std::move(stream));
    if (typeface)
      ReportWebFontInstantiationResult(kSuccessCbdtCblcColorFont);
  }

  if (format_check.IsSbixColorFont()) {
    typeface = FontManagerForSbix()->makeFromStream(std::move(stream));
    if (typeface) {
      ReportWebFontInstantiationResult(kSuccessSbixFont);
    }
  }

  if (format_check.IsCff2OutlineFont()) {
    typeface = FreeTypeFontManager()->makeFromStream(std::move(stream));
    if (typeface)
      ReportWebFontInstantiationResult(kSuccessCff2Font);
  }

  // Variable CFF2 fonts must go through FreeType.
  if (format_check.IsVariableFont() && !format_check.IsCff2OutlineFont()) {
    typeface = FontManagerForVariations()->makeFromStream(std::move(stream));
    if (typeface)
      ReportWebFontInstantiationResult(kSuccessVariableWebFont);
    else
      ReportWebFontInstantiationResult(kErrorInstantiatingVariableFont);
  }

  return true;
}

sk_sp<SkFontMgr> WebFontTypefaceFactory::FontManagerForVariations() {
#if defined(OS_WIN)
  return FreeTypeFontManager();
#else
#if defined(OS_MACOSX)
  if (!CoreTextVersionSupportsVariations())
    return FreeTypeFontManager();
#endif
  return DefaultFontManager();
#endif
}

sk_sp<SkFontMgr> WebFontTypefaceFactory::FontManagerForSbix() {
#if defined(OS_MACOSX)
  return DefaultFontManager();
#endif
  return FreeTypeFontManager();
}

sk_sp<SkFontMgr> WebFontTypefaceFactory::DefaultFontManager() {
#if defined(OS_WIN)
  return FontCache::GetFontCache()->FontManager();
#else
  return sk_sp<SkFontMgr>(SkFontMgr::RefDefault());
#endif
}

sk_sp<SkFontMgr> WebFontTypefaceFactory::FreeTypeFontManager() {
#if defined(OS_WIN) || defined(OS_MACOSX)
  return sk_sp<SkFontMgr>(SkFontMgr_New_Custom_Empty());
#else
  return DefaultFontManager();
#endif
}

void WebFontTypefaceFactory::ReportWebFontInstantiationResult(
    WebFontInstantiationResult result) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      EnumerationHistogram, web_font_variable_fonts_ratio,
      ("Blink.Fonts.VariableFontsRatio", kMaxWebFontInstantiationResult));
  web_font_variable_fonts_ratio.Count(result);
}

}  // namespace blink
