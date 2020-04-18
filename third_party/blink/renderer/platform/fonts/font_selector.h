/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_FONT_SELECTOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_FONT_SELECTOR_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/fonts/font_cache_client.h"
#include "third_party/blink/renderer/platform/fonts/segmented_font_data.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class ExecutionContext;
class FontData;
class FontDescription;
class FontFaceCache;
class FontSelectorClient;
class GenericFontFamilySettings;

class PLATFORM_EXPORT FontSelector : public FontCacheClient {
 public:
  ~FontSelector() override = default;
  virtual scoped_refptr<FontData> GetFontData(const FontDescription&,
                                       const AtomicString& family_name) = 0;

  // TODO crbug.com/542629 - The String variant of this method shouldbe replaced
  // with a better approach, now that we only have complex text.
  virtual void WillUseFontData(const FontDescription&,
                               const AtomicString& family_name,
                               const String& text) = 0;
  virtual void WillUseRange(const FontDescription&,
                            const AtomicString& family_name,
                            const FontDataForRangeSet&) = 0;

  virtual unsigned Version() const = 0;

  virtual void ReportNotDefGlyph() const = 0;

  virtual void RegisterForInvalidationCallbacks(FontSelectorClient*) = 0;
  virtual void UnregisterForInvalidationCallbacks(FontSelectorClient*) = 0;

  virtual void FontFaceInvalidated(){};

  virtual ExecutionContext* GetExecutionContext() const = 0;

  virtual FontFaceCache* GetFontFaceCache() = 0;

  virtual bool IsPlatformFamilyMatchAvailable(
      const FontDescription&,
      const AtomicString& passed_family) = 0;

 protected:
  static AtomicString FamilyNameFromSettings(
      const GenericFontFamilySettings&,
      const FontDescription&,
      const AtomicString& generic_family_name);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_FONT_SELECTOR_H_
