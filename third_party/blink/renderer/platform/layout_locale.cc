// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/layout_locale.h"

#include "third_party/blink/renderer/platform/fonts/accept_languages_resolver.h"
#include "third_party/blink/renderer/platform/fonts/font_global_context.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/blink/renderer/platform/text/icu_error.h"
#include "third_party/blink/renderer/platform/text/locale_to_script_mapping.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_hash.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"

#include <hb.h>
#include <unicode/locid.h>

namespace blink {

static hb_language_t ToHarfbuzLanguage(const AtomicString& locale) {
  CString locale_as_latin1 = locale.Latin1();
  return hb_language_from_string(locale_as_latin1.data(),
                                 locale_as_latin1.length());
}

// SkFontMgr requires script-based locale names, like "zh-Hant" and "zh-Hans",
// instead of "zh-CN" and "zh-TW".
static const char* ToSkFontMgrLocale(UScriptCode script) {
  switch (script) {
    case USCRIPT_KATAKANA_OR_HIRAGANA:
      return "ja-JP";
    case USCRIPT_HANGUL:
      return "ko-KR";
    case USCRIPT_SIMPLIFIED_HAN:
      return "zh-Hans";
    case USCRIPT_TRADITIONAL_HAN:
      return "zh-Hant";
    default:
      return nullptr;
  }
}

const char* LayoutLocale::LocaleForSkFontMgr() const {
  if (string_for_sk_font_mgr_.IsNull()) {
    string_for_sk_font_mgr_ = ToSkFontMgrLocale(script_);
    if (string_for_sk_font_mgr_.IsNull())
      string_for_sk_font_mgr_ = string_.Ascii();
  }
  return string_for_sk_font_mgr_.data();
}

void LayoutLocale::ComputeScriptForHan() const {
  if (IsUnambiguousHanScript(script_)) {
    script_for_han_ = script_;
    has_script_for_han_ = true;
    return;
  }

  script_for_han_ = ScriptCodeForHanFromSubtags(string_);
  if (script_for_han_ == USCRIPT_COMMON)
    script_for_han_ = USCRIPT_SIMPLIFIED_HAN;
  else
    has_script_for_han_ = true;
  DCHECK(IsUnambiguousHanScript(script_for_han_));
}

UScriptCode LayoutLocale::GetScriptForHan() const {
  if (script_for_han_ == USCRIPT_COMMON)
    ComputeScriptForHan();
  return script_for_han_;
}

bool LayoutLocale::HasScriptForHan() const {
  if (script_for_han_ == USCRIPT_COMMON)
    ComputeScriptForHan();
  return has_script_for_han_;
}

const LayoutLocale* LayoutLocale::LocaleForHan(
    const LayoutLocale* content_locale) {
  if (content_locale && content_locale->HasScriptForHan())
    return content_locale;

  if (FontGlobalContext::HasDefaultLocaleForHan())
    return FontGlobalContext::GetDefaultLocaleForHan();

  const LayoutLocale* default_for_han;
  if (const LayoutLocale* locale = AcceptLanguagesResolver::LocaleForHan())
    default_for_han = locale;
  else if (GetDefault().HasScriptForHan())
    default_for_han = &GetDefault();
  else if (GetSystem().HasScriptForHan())
    default_for_han = &GetSystem();
  else
    default_for_han = nullptr;
  FontGlobalContext::SetDefaultLocaleForHan(default_for_han);
  return default_for_han;
}

const char* LayoutLocale::LocaleForHanForSkFontMgr() const {
  const char* locale = ToSkFontMgrLocale(GetScriptForHan());
  DCHECK(locale);
  return locale;
}

LayoutLocale::LayoutLocale(const AtomicString& locale)
    : string_(locale),
      harfbuzz_language_(ToHarfbuzLanguage(locale)),
      script_(LocaleToScriptCodeForFontSelection(locale)),
      script_for_han_(USCRIPT_COMMON),
      has_script_for_han_(false),
      hyphenation_computed_(false) {}

const LayoutLocale* LayoutLocale::Get(const AtomicString& locale) {
  if (locale.IsNull())
    return nullptr;

  auto result = FontGlobalContext::GetLayoutLocaleMap().insert(locale, nullptr);
  if (result.is_new_entry)
    result.stored_value->value = base::AdoptRef(new LayoutLocale(locale));
  return result.stored_value->value.get();
}

const LayoutLocale& LayoutLocale::GetDefault() {
  if (const LayoutLocale* locale = FontGlobalContext::GetDefaultLayoutLocale())
    return *locale;

  AtomicString language = DefaultLanguage();
  const LayoutLocale* locale =
      LayoutLocale::Get(!language.IsEmpty() ? language : "en");
  FontGlobalContext::SetDefaultLayoutLocale(locale);
  return *locale;
}

const LayoutLocale& LayoutLocale::GetSystem() {
  if (const LayoutLocale* locale = FontGlobalContext::GetSystemLayoutLocale())
    return *locale;

  // Platforms such as Windows can give more information than the default
  // locale, such as "en-JP" for English speakers in Japan.
  String name = icu::Locale::getDefault().getName();
  const LayoutLocale* locale =
      LayoutLocale::Get(AtomicString(name.Replace('_', '-')));
  FontGlobalContext::SetSystemLayoutLocale(locale);
  return *locale;
}

scoped_refptr<LayoutLocale> LayoutLocale::CreateForTesting(
    const AtomicString& locale) {
  return base::AdoptRef(new LayoutLocale(locale));
}

Hyphenation* LayoutLocale::GetHyphenation() const {
  if (hyphenation_computed_)
    return hyphenation_.get();

  hyphenation_computed_ = true;
  hyphenation_ = Hyphenation::PlatformGetHyphenation(LocaleString());
  return hyphenation_.get();
}

void LayoutLocale::SetHyphenationForTesting(
    const AtomicString& locale_string,
    scoped_refptr<Hyphenation> hyphenation) {
  const LayoutLocale& locale = ValueOrDefault(Get(locale_string));
  locale.hyphenation_computed_ = true;
  locale.hyphenation_ = std::move(hyphenation);
}

AtomicString LayoutLocale::LocaleWithBreakKeyword(
    LineBreakIteratorMode mode) const {
  if (string_.IsEmpty())
    return string_;

  // uloc_setKeywordValue_58 has a problem to handle "@" in the original
  // string. crbug.com/697859
  if (string_.Contains('@'))
    return string_;

  CString utf8_locale = string_.Utf8();
  Vector<char> buffer(utf8_locale.length() + 11, 0);
  memcpy(buffer.data(), utf8_locale.data(), utf8_locale.length());

  const char* keyword_value = nullptr;
  switch (mode) {
    default:
      NOTREACHED();
      FALLTHROUGH;
    case LineBreakIteratorMode::kDefault:
      // nullptr will cause any existing values to be removed.
      break;
    case LineBreakIteratorMode::kNormal:
      keyword_value = "normal";
      break;
    case LineBreakIteratorMode::kStrict:
      keyword_value = "strict";
      break;
    case LineBreakIteratorMode::kLoose:
      keyword_value = "loose";
      break;
  }

  ICUError status;
  int32_t length_needed = uloc_setKeywordValue(
      "lb", keyword_value, buffer.data(), buffer.size(), &status);
  if (U_SUCCESS(status))
    return AtomicString::FromUTF8(buffer.data(), length_needed);

  if (status == U_BUFFER_OVERFLOW_ERROR) {
    buffer.Grow(length_needed + 1);
    memset(buffer.data() + utf8_locale.length(), 0,
           buffer.size() - utf8_locale.length());
    status = U_ZERO_ERROR;
    int32_t length_needed2 = uloc_setKeywordValue(
        "lb", keyword_value, buffer.data(), buffer.size(), &status);
    DCHECK_EQ(length_needed, length_needed2);
    if (U_SUCCESS(status) && length_needed == length_needed2)
      return AtomicString::FromUTF8(buffer.data(), length_needed);
  }

  NOTREACHED();
  return string_;
}

}  // namespace blink
