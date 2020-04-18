// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_SHUTDOWN_NOTIFIER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_SHUTDOWN_NOTIFIER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/lifecycle_notifier.h"

namespace blink {

class Document;
class DocumentShutdownObserver;

// Sibling class of DocumentShutdownObserver; implemented by Document to notify
// subclasses of DocumentShutdownObserver of Document shutdown.
class CORE_EXPORT DocumentShutdownNotifier
    : public LifecycleNotifier<Document, DocumentShutdownObserver> {
 protected:
  DocumentShutdownNotifier();

 private:
  DISALLOW_COPY_AND_ASSIGN(DocumentShutdownNotifier);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_SHUTDOWN_NOTIFIER_H_
