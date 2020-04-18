/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 * Copyright (C) 2012, Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_NAVIGATOR_CONTENT_UTILS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_NAVIGATOR_CONTENT_UTILS_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/navigatorcontentutils/navigator_content_utils_client.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ExceptionState;

class MODULES_EXPORT NavigatorContentUtils final
    : public GarbageCollectedFinalized<NavigatorContentUtils>,
      public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorContentUtils);

 public:
  static const char kSupplementName[];

  virtual ~NavigatorContentUtils();

  static NavigatorContentUtils* From(Navigator&);

  static void registerProtocolHandler(Navigator&,
                                      const String& scheme,
                                      const String& url,
                                      const String& title,
                                      ExceptionState&);
  static void unregisterProtocolHandler(Navigator&,
                                        const String& scheme,
                                        const String& url,
                                        ExceptionState&);

  static void ProvideTo(Navigator&, NavigatorContentUtilsClient*);

  void Trace(blink::Visitor*) override;

  void SetClientForTest(NavigatorContentUtilsClient* client) {
    client_ = client;
  }

 private:
  NavigatorContentUtils(Navigator& navigator,
                        NavigatorContentUtilsClient* client)
      : Supplement<Navigator>(navigator), client_(client) {}

  NavigatorContentUtilsClient* Client() { return client_.Get(); }

  Member<NavigatorContentUtilsClient> client_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_NAVIGATOR_CONTENT_UTILS_H_
