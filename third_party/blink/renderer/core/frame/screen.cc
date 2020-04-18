/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/frame/screen.h"

#include "third_party/blink/public/platform/web_screen_info.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

namespace blink {

Screen::Screen(LocalFrame* frame) : DOMWindowClient(frame) {}

int Screen::height() const {
  if (!GetFrame())
    return 0;
  Page* page = GetFrame()->GetPage();
  if (!page)
    return 0;
  if (page->GetSettings().GetReportScreenSizeInPhysicalPixelsQuirk()) {
    WebScreenInfo screen_info = page->GetChromeClient().GetScreenInfo();
    return lroundf(screen_info.rect.height * screen_info.device_scale_factor);
  }
  return page->GetChromeClient().GetScreenInfo().rect.height;
}

int Screen::width() const {
  if (!GetFrame())
    return 0;
  Page* page = GetFrame()->GetPage();
  if (!page)
    return 0;
  if (page->GetSettings().GetReportScreenSizeInPhysicalPixelsQuirk()) {
    WebScreenInfo screen_info = page->GetChromeClient().GetScreenInfo();
    return lroundf(screen_info.rect.width * screen_info.device_scale_factor);
  }
  return page->GetChromeClient().GetScreenInfo().rect.width;
}

unsigned Screen::colorDepth() const {
  if (!GetFrame() || !GetFrame()->GetPage())
    return 0;
  return static_cast<unsigned>(
      GetFrame()->GetPage()->GetChromeClient().GetScreenInfo().depth);
}

unsigned Screen::pixelDepth() const {
  if (!GetFrame())
    return 0;
  return static_cast<unsigned>(
      GetFrame()->GetPage()->GetChromeClient().GetScreenInfo().depth);
}

int Screen::availLeft() const {
  if (!GetFrame())
    return 0;
  Page* page = GetFrame()->GetPage();
  if (!page)
    return 0;
  if (page->GetSettings().GetReportScreenSizeInPhysicalPixelsQuirk()) {
    WebScreenInfo screen_info = page->GetChromeClient().GetScreenInfo();
    return lroundf(screen_info.available_rect.x *
                   screen_info.device_scale_factor);
  }
  return static_cast<int>(
      page->GetChromeClient().GetScreenInfo().available_rect.x);
}

int Screen::availTop() const {
  if (!GetFrame())
    return 0;
  Page* page = GetFrame()->GetPage();
  if (!page)
    return 0;
  if (page->GetSettings().GetReportScreenSizeInPhysicalPixelsQuirk()) {
    WebScreenInfo screen_info = page->GetChromeClient().GetScreenInfo();
    return lroundf(screen_info.available_rect.y *
                   screen_info.device_scale_factor);
  }
  return static_cast<int>(
      page->GetChromeClient().GetScreenInfo().available_rect.y);
}

int Screen::availHeight() const {
  if (!GetFrame())
    return 0;
  Page* page = GetFrame()->GetPage();
  if (!page)
    return 0;
  if (page->GetSettings().GetReportScreenSizeInPhysicalPixelsQuirk()) {
    WebScreenInfo screen_info = page->GetChromeClient().GetScreenInfo();
    return lroundf(screen_info.available_rect.height *
                   screen_info.device_scale_factor);
  }
  return page->GetChromeClient().GetScreenInfo().available_rect.height;
}

int Screen::availWidth() const {
  if (!GetFrame())
    return 0;
  Page* page = GetFrame()->GetPage();
  if (!page)
    return 0;
  if (page->GetSettings().GetReportScreenSizeInPhysicalPixelsQuirk()) {
    WebScreenInfo screen_info = page->GetChromeClient().GetScreenInfo();
    return lroundf(screen_info.available_rect.width *
                   screen_info.device_scale_factor);
  }
  return page->GetChromeClient().GetScreenInfo().available_rect.width;
}

void Screen::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  DOMWindowClient::Trace(visitor);
  Supplementable<Screen>::Trace(visitor);
}

}  // namespace blink
