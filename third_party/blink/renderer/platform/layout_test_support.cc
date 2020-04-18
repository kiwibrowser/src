/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/platform/layout_test_support.h"

#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

// Wrapper functions defined in WebKit.h
void SetLayoutTestMode(bool value) {
  LayoutTestSupport::SetIsRunningLayoutTest(value);
}

bool LayoutTestMode() {
  return LayoutTestSupport::IsRunningLayoutTest();
}

void SetFontAntialiasingEnabledForTest(bool value) {
  LayoutTestSupport::SetFontAntialiasingEnabledForTest(value);
}

bool FontAntialiasingEnabledForTest() {
  return LayoutTestSupport::IsFontAntialiasingEnabledForTest();
}

static bool g_is_running_layout_test = false;
static bool g_is_mock_theme_enabled = false;
static bool g_is_font_antialiasing_enabled = false;
static bool g_is_subpixel_positioning_allowed = true;

bool LayoutTestSupport::IsRunningLayoutTest() {
  return g_is_running_layout_test;
}

void LayoutTestSupport::SetIsRunningLayoutTest(bool value) {
  g_is_running_layout_test = value;
}

bool LayoutTestSupport::IsMockThemeEnabledForTest() {
  return g_is_mock_theme_enabled;
}

void LayoutTestSupport::SetMockThemeEnabledForTest(bool value) {
  DCHECK(g_is_running_layout_test);
  g_is_mock_theme_enabled = value;
}

bool LayoutTestSupport::IsFontAntialiasingEnabledForTest() {
  return g_is_font_antialiasing_enabled;
}

void LayoutTestSupport::SetFontAntialiasingEnabledForTest(bool value) {
  g_is_font_antialiasing_enabled = value;
}

bool LayoutTestSupport::IsTextSubpixelPositioningAllowedForTest() {
  return g_is_subpixel_positioning_allowed;
}

void LayoutTestSupport::SetTextSubpixelPositioningAllowedForTest(bool value) {
  g_is_subpixel_positioning_allowed = value;
}

}  // namespace blink
