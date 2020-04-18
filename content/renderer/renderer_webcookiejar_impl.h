// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RENDERER_WEBCOOKIEJAR_IMPL_H_
#define CONTENT_RENDERER_RENDERER_WEBCOOKIEJAR_IMPL_H_

// TODO(darin): WebCookieJar.h is missing a WebString.h include!
#include "third_party/blink/public/platform/web_cookie_jar.h"

namespace content {
class RenderFrameImpl;

class RendererWebCookieJarImpl : public blink::WebCookieJar {
 public:
  explicit RendererWebCookieJarImpl(RenderFrameImpl* sender)
      : sender_(sender) {
  }
  virtual ~RendererWebCookieJarImpl() {}

 private:
  // blink::WebCookieJar methods:
  void SetCookie(const blink::WebURL& url,
                 const blink::WebURL& site_for_cookies,
                 const blink::WebString& value) override;
  blink::WebString Cookies(const blink::WebURL& url,
                           const blink::WebURL& site_for_cookies) override;
  bool CookiesEnabled(const blink::WebURL& url,
                      const blink::WebURL& site_for_cookies) override;

  RenderFrameImpl* sender_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_RENDERER_WEBCOOKIEJAR_IMPL_H_
