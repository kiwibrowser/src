// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/utility/utility_blink_platform_with_sandbox_support_impl.h"

#include "build/build_config.h"

#if defined(OS_MACOSX)
#include "base/mac/foundation_util.h"
#include "content/child/child_process_sandbox_support_impl_mac.h"
#include "third_party/blink/public/platform/mac/web_sandbox_support.h"
#elif defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)
#include "base/synchronization/lock.h"
#include "content/child/child_process_sandbox_support_impl_linux.h"
#include "third_party/blink/public/platform/linux/web_fallback_font.h"
#include "third_party/blink/public/platform/linux/web_sandbox_support.h"
#endif

namespace blink {
class WebSandboxSupport;
struct WebFallbackFont;
struct WebFontRenderStyle;
}  // namespace blink

namespace content {

#if defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)

class UtilityBlinkPlatformWithSandboxSupportImpl::SandboxSupport
    : public blink::WebSandboxSupport {
 public:
  ~SandboxSupport() override {}

#if defined(OS_MACOSX)
  bool LoadFont(CTFontRef srcFont, CGFontRef* out, uint32_t* fontID) override;
#else
  void GetFallbackFontForCharacter(
      blink::WebUChar32 character,
      const char* preferred_locale,
      blink::WebFallbackFont* fallbackFont) override;
  void GetWebFontRenderStyleForStrike(const char* family,
                                      int sizeAndStyle,
                                      blink::WebFontRenderStyle* out) override;

 private:
  // WebKit likes to ask us for the correct font family to use for a set of
  // unicode code points. It needs this information frequently so we cache it
  // here.
  base::Lock unicode_font_families_mutex_;
  // Maps unicode chars to their fallback fonts.
  std::map<int32_t, blink::WebFallbackFont> unicode_font_families_;
#endif  // defined(OS_MACOSX)
};

#endif  // defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)

UtilityBlinkPlatformWithSandboxSupportImpl::
    UtilityBlinkPlatformWithSandboxSupportImpl() {
#if defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)
  sandbox_support_ = std::make_unique<SandboxSupport>();
#endif
}

UtilityBlinkPlatformWithSandboxSupportImpl::
    ~UtilityBlinkPlatformWithSandboxSupportImpl() {}

blink::WebSandboxSupport*
UtilityBlinkPlatformWithSandboxSupportImpl::GetSandboxSupport() {
#if defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)
  return sandbox_support_.get();
#else
  return nullptr;
#endif
}

#if defined(OS_MACOSX)

bool UtilityBlinkPlatformWithSandboxSupportImpl::SandboxSupport::LoadFont(
    CTFontRef src_font,
    CGFontRef* out,
    uint32_t* font_id) {
  return content::LoadFont(src_font, out, font_id);
}

#elif defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)

void UtilityBlinkPlatformWithSandboxSupportImpl::SandboxSupport::
    GetFallbackFontForCharacter(blink::WebUChar32 character,
                                const char* preferred_locale,
                                blink::WebFallbackFont* fallback_font) {
  base::AutoLock lock(unicode_font_families_mutex_);
  const std::map<int32_t, blink::WebFallbackFont>::const_iterator iter =
      unicode_font_families_.find(character);
  if (iter != unicode_font_families_.end()) {
    fallback_font->name = iter->second.name;
    fallback_font->filename = iter->second.filename;
    fallback_font->fontconfig_interface_id =
        iter->second.fontconfig_interface_id;
    fallback_font->ttc_index = iter->second.ttc_index;
    fallback_font->is_bold = iter->second.is_bold;
    fallback_font->is_italic = iter->second.is_italic;
    return;
  }

  content::GetFallbackFontForCharacter(character, preferred_locale,
                                       fallback_font);
  unicode_font_families_.emplace(character, *fallback_font);
}

void UtilityBlinkPlatformWithSandboxSupportImpl::SandboxSupport::
    GetWebFontRenderStyleForStrike(const char* family,
                                   int size_and_style,
                                   blink::WebFontRenderStyle* out) {
  GetRenderStyleForStrike(family, size_and_style, out);
}

#endif

}  // namespace content
