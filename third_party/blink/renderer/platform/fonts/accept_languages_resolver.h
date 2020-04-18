// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_ACCEPT_LANGUAGES_RESOLVER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_ACCEPT_LANGUAGES_RESOLVER_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

#include <unicode/uscript.h>

namespace blink {

class LayoutLocale;

class PLATFORM_EXPORT AcceptLanguagesResolver {
 public:
  static void AcceptLanguagesChanged(const String&);

  static const LayoutLocale* LocaleForHan();
  static const LayoutLocale* LocaleForHanFromAcceptLanguages(const String&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_ACCEPT_LANGUAGES_RESOLVER_H_
