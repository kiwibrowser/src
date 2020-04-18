// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_DOM_WINDOW_STORAGE_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_DOM_WINDOW_STORAGE_CONTROLLER_H_

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Document;

class MODULES_EXPORT DOMWindowStorageController final
    : public GarbageCollected<DOMWindowStorageController>,
      public Supplement<Document>,
      public LocalDOMWindow::EventListenerObserver {
  USING_GARBAGE_COLLECTED_MIXIN(DOMWindowStorageController);

 public:
  static const char kSupplementName[];

  void Trace(blink::Visitor*) override;

  static DOMWindowStorageController& From(Document&);

  // Inherited from LocalDOMWindow::EventListenerObserver
  void DidAddEventListener(LocalDOMWindow*, const AtomicString&) override;
  void DidRemoveEventListener(LocalDOMWindow*, const AtomicString&) override {}
  void DidRemoveAllEventListeners(LocalDOMWindow*) override {}

 private:
  explicit DOMWindowStorageController(Document&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_DOM_WINDOW_STORAGE_CONTROLLER_H_
