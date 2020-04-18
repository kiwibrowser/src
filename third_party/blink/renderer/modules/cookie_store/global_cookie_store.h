// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_GLOBAL_COOKIE_STORE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_GLOBAL_COOKIE_STORE_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class CookieStore;
class LocalDOMWindow;
class ServiceWorkerGlobalScope;

// Exposes a CookieStore as the "cookieStore" attribute on the global scope.
//
// This currently applies to Window scopes.
class GlobalCookieStore {
  STATIC_ONLY(GlobalCookieStore);

 public:
  static CookieStore* cookieStore(LocalDOMWindow&);
  static CookieStore* cookieStore(ServiceWorkerGlobalScope&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_GLOBAL_COOKIE_STORE_H_
