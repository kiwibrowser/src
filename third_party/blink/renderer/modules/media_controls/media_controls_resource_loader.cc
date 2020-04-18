// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/media_controls_resource_loader.h"

#include "build/build_config.h"
#include "third_party/blink/public/resources/grit/media_controls_resources.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/media/resource_bundle_helper.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace {

bool ShouldLoadAndroidCSS() {
#if defined(OS_ANDROID)
  return true;
#else
  return blink::RuntimeEnabledFeatures::MobileLayoutThemeEnabled();
#endif
}

}  // namespace

namespace blink {

MediaControlsResourceLoader::MediaControlsResourceLoader()
    : UAStyleSheetLoader() {}

MediaControlsResourceLoader::~MediaControlsResourceLoader() = default;

String MediaControlsResourceLoader::GetMediaControlsCSS() const {
  return ResourceBundleHelper::UncompressResourceAsString(
      RuntimeEnabledFeatures::ModernMediaControlsEnabled()
          ? IDR_UASTYLE_MODERN_MEDIA_CONTROLS_CSS
          : IDR_UASTYLE_LEGACY_MEDIA_CONTROLS_CSS);
};

String MediaControlsResourceLoader::GetMediaControlsAndroidCSS() const {
  if (RuntimeEnabledFeatures::ModernMediaControlsEnabled())
    return String();
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_UASTYLE_LEGACY_MEDIA_CONTROLS_ANDROID_CSS);
};

// static
String MediaControlsResourceLoader::GetShadowTimelineStyleSheet() {
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_SHADOWSTYLE_MODERN_MEDIA_CONTROLS_TIMELINE_CSS);
};

// static
String MediaControlsResourceLoader::GetShadowLoadingStyleSheet() {
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_SHADOWSTYLE_MODERN_MEDIA_CONTROLS_LOADING_CSS);
};

// static
String MediaControlsResourceLoader::GetJumpSVGImage() {
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_MODERN_MEDIA_CONTROLS_JUMP_SVG);
};

// static
String MediaControlsResourceLoader::GetArrowRightSVGImage() {
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_MODERN_MEDIA_CONTROLS_ARROW_RIGHT_SVG);
};

// static
String MediaControlsResourceLoader::GetArrowLeftSVGImage() {
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_MODERN_MEDIA_CONTROLS_ARROW_LEFT_SVG);
};

// static
String MediaControlsResourceLoader::GetScrubbingMessageStyleSheet() {
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_SHADOWSTYLE_MODERN_MEDIA_CONTROLS_SCRUBBING_MESSAGE_CSS);
};

// static
String MediaControlsResourceLoader::GetOverlayPlayStyleSheet() {
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_SHADOWSTYLE_MODERN_MEDIA_CONTROLS_OVERLAY_PLAY_CSS);
};

// static
String MediaControlsResourceLoader::GetMediaInterstitialsStyleSheet() {
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_UASTYLE_MEDIA_INTERSTITIALS_CSS);
};

String MediaControlsResourceLoader::GetUAStyleSheet() {
  if (ShouldLoadAndroidCSS()) {
    return GetMediaControlsCSS() + GetMediaControlsAndroidCSS() +
           GetMediaInterstitialsStyleSheet();
  }
  return GetMediaControlsCSS() + GetMediaInterstitialsStyleSheet();
}

void MediaControlsResourceLoader::InjectMediaControlsUAStyleSheet() {
  CSSDefaultStyleSheets& default_style_sheets =
      CSSDefaultStyleSheets::Instance();
  std::unique_ptr<MediaControlsResourceLoader> loader =
      std::make_unique<MediaControlsResourceLoader>();

  if (!default_style_sheets.HasMediaControlsStyleSheetLoader())
    default_style_sheets.SetMediaControlsStyleSheetLoader(std::move(loader));
}

}  // namespace blink
