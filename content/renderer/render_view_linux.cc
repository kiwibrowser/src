// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/render_view_impl.h"

#include "content/public/common/renderer_preferences.h"
#include "third_party/blink/public/platform/web_font_render_style.h"
#include "third_party/skia/include/core/SkFontLCDConfig.h"
#include "ui/gfx/font_render_params.h"

using blink::WebFontRenderStyle;

namespace content {

namespace {

SkPaint::Hinting RendererPreferencesToSkiaHinting(
    const RendererPreferences& prefs) {
  if (!prefs.should_antialias_text) {
    // When anti-aliasing is off, GTK maps all non-zero hinting settings to
    // 'Normal' hinting so we do the same. Otherwise, folks who have 'Slight'
    // hinting selected will see readable text in everything expect Chromium.
    switch (prefs.hinting) {
      case gfx::FontRenderParams::HINTING_NONE:
        return SkPaint::kNo_Hinting;
      case gfx::FontRenderParams::HINTING_SLIGHT:
      case gfx::FontRenderParams::HINTING_MEDIUM:
      case gfx::FontRenderParams::HINTING_FULL:
        return SkPaint::kNormal_Hinting;
      default:
        NOTREACHED();
        return SkPaint::kNormal_Hinting;
    }
  }

  switch (prefs.hinting) {
    case gfx::FontRenderParams::HINTING_NONE:   return SkPaint::kNo_Hinting;
    case gfx::FontRenderParams::HINTING_SLIGHT: return SkPaint::kSlight_Hinting;
    case gfx::FontRenderParams::HINTING_MEDIUM: return SkPaint::kNormal_Hinting;
    case gfx::FontRenderParams::HINTING_FULL:   return SkPaint::kFull_Hinting;
    default:
      NOTREACHED();
      return SkPaint::kNormal_Hinting;
    }
}

}  // namespace

void RenderViewImpl::UpdateFontRenderingFromRendererPrefs() {
  const RendererPreferences& prefs = renderer_preferences_;
  WebFontRenderStyle::SetHinting(RendererPreferencesToSkiaHinting(prefs));
  WebFontRenderStyle::SetAutoHint(prefs.use_autohinter);
  WebFontRenderStyle::SetUseBitmaps(prefs.use_bitmaps);
  SkFontLCDConfig::SetSubpixelOrder(
      gfx::FontRenderParams::SubpixelRenderingToSkiaLCDOrder(
          prefs.subpixel_rendering));
  SkFontLCDConfig::SetSubpixelOrientation(
      gfx::FontRenderParams::SubpixelRenderingToSkiaLCDOrientation(
          prefs.subpixel_rendering));
  WebFontRenderStyle::SetAntiAlias(prefs.should_antialias_text);
  WebFontRenderStyle::SetSubpixelRendering(
      prefs.subpixel_rendering !=
      gfx::FontRenderParams::SUBPIXEL_RENDERING_NONE);
  WebFontRenderStyle::SetSubpixelPositioning(prefs.use_subpixel_positioning);
#if !defined(OS_ANDROID)
  if (!prefs.system_font_family_name.empty()) {
    WebFontRenderStyle::SetSystemFontFamily(
        blink::WebString::FromUTF8(prefs.system_font_family_name));
  }
#endif
}

}  // namespace content
