// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_HTML_IFRAME_ELEMENT_PAYMENTS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_HTML_IFRAME_ELEMENT_PAYMENTS_H_

#include "third_party/blink/renderer/core/html/html_iframe_element.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class HTMLIFrameElement;
class QualifiedName;

class HTMLIFrameElementPayments final
    : public GarbageCollected<HTMLIFrameElementPayments>,
      public Supplement<HTMLIFrameElement> {
  USING_GARBAGE_COLLECTED_MIXIN(HTMLIFrameElementPayments);

 public:
  static const char kSupplementName[];

  static bool FastHasAttribute(const QualifiedName&, const HTMLIFrameElement&);
  static void SetBooleanAttribute(const QualifiedName&,
                                  HTMLIFrameElement&,
                                  bool);
  static HTMLIFrameElementPayments& From(HTMLIFrameElement&);
  static bool AllowPaymentRequest(HTMLIFrameElement&);

  void Trace(blink::Visitor*) override;

 private:
  HTMLIFrameElementPayments();
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_HTML_IFRAME_ELEMENT_PAYMENTS_H_
