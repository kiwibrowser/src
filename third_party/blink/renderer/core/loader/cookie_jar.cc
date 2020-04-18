/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/loader/cookie_jar.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_cookie_jar.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/platform/histogram.h"

namespace blink {

static WebCookieJar* ToCookieJar(const Document* document) {
  if (!document || !document->GetFrame())
    return nullptr;
  return document->GetFrame()->Client()->CookieJar();
}

String Cookies(const Document* document, const KURL& url) {
  WebCookieJar* cookie_jar = ToCookieJar(document);
  if (!cookie_jar)
    return String();

  SCOPED_BLINK_UMA_HISTOGRAM_TIMER("Blink.CookieJar.SyncCookiesTime");
  return cookie_jar->Cookies(url, document->SiteForCookies());
}

void SetCookies(Document* document,
                const KURL& url,
                const String& cookie_string) {
  WebCookieJar* cookie_jar = ToCookieJar(document);
  if (!cookie_jar)
    return;
  SCOPED_BLINK_UMA_HISTOGRAM_TIMER("Blink.CookieJar.SyncCookiesSetTime");
  cookie_jar->SetCookie(url, document->SiteForCookies(), cookie_string);
}

bool CookiesEnabled(const Document* document) {
  WebCookieJar* cookie_jar = ToCookieJar(document);
  if (!cookie_jar)
    return false;
  return cookie_jar->CookiesEnabled(document->CookieURL(),
                                    document->SiteForCookies());
}

}  // namespace blink
