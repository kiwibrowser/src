// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/accept_languages_resolver.h"

#include "third_party/blink/renderer/platform/fonts/font_global_context.h"
#include "third_party/blink/renderer/platform/layout_locale.h"

namespace blink {

void AcceptLanguagesResolver::AcceptLanguagesChanged(
    const String& accept_languages) {
  String& current_value = FontGlobalContext::CurrentAcceptLanguages();
  if (current_value == accept_languages)
    return;

  current_value = accept_languages;
  FontGlobalContext::InvalidateLocaleForHan();
}

const LayoutLocale* AcceptLanguagesResolver::LocaleForHan() {
  return LocaleForHanFromAcceptLanguages(
      FontGlobalContext::CurrentAcceptLanguages());
}

const LayoutLocale* AcceptLanguagesResolver::LocaleForHanFromAcceptLanguages(
    const String& accept_languages) {
  // Use the first acceptLanguages that can disambiguate.
  Vector<String> languages;
  accept_languages.Split(',', languages);
  for (String token : languages) {
    token = token.StripWhiteSpace();
    const LayoutLocale* locale = LayoutLocale::Get(AtomicString(token));
    if (locale->HasScriptForHan())
      return locale;
  }

  return nullptr;
}

}  // namespace blink
