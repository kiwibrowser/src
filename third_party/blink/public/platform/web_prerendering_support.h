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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_PRERENDERING_SUPPORT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_PRERENDERING_SUPPORT_H_

#include "third_party/blink/public/platform/web_common.h"

namespace blink {

class WebPrerender;

class WebPrerenderingSupport {
 public:
  BLINK_PLATFORM_EXPORT static void Initialize(WebPrerenderingSupport*);
  BLINK_PLATFORM_EXPORT static void Shutdown();
  BLINK_PLATFORM_EXPORT static WebPrerenderingSupport* Current();

  // A prerender link element is added when it is inserted into a document.
  virtual void Add(const WebPrerender&) = 0;

  // A prerender is canceled when it is removed from a document.
  virtual void Cancel(const WebPrerender&) = 0;

  // A prerender is abandoned when it's navigated away from or suspended in the
  // page cache. This is a weaker signal than Cancel(), since the launcher
  // hasn't indicated that the prerender isn't wanted, and we may end up using
  // it after, for instance, a short redirect chain.
  virtual void Abandon(const WebPrerender&) = 0;

  // Called when the current page has finished requesting early discoverable
  // resources for prefetch. In prefetch mode link elements do not initiate any
  // prerenders.
  virtual void PrefetchFinished() = 0;

 protected:
  WebPrerenderingSupport() = default;
  virtual ~WebPrerenderingSupport() = default;

 private:
  static WebPrerenderingSupport* platform_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_PRERENDERING_SUPPORT_H_
