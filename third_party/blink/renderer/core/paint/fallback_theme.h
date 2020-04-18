// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FALLBACK_THEME_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FALLBACK_THEME_H_

namespace ui {
class NativeTheme;
}  // namespace ui

namespace blink {

// Gets the fallback theme, which is used to paint checkboxes and radio buttons
// when |LayoutTheme::ShouldUseFallbackTheme| returns true.
ui::NativeTheme& GetFallbackTheme();

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FALLBACK_THEME_H_
