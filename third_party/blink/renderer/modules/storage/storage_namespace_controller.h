// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_NAMESPACE_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_NAMESPACE_CONTROLLER_H_

#include <memory>

#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/storage/storage_area.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class InspectorDOMStorageAgent;
class StorageNamespace;
class WebViewClient;

class MODULES_EXPORT StorageNamespaceController final
    : public GarbageCollectedFinalized<StorageNamespaceController>,
      public Supplement<Page> {
  USING_GARBAGE_COLLECTED_MIXIN(StorageNamespaceController);

 public:
  static const char kSupplementName[];

  StorageNamespace* SessionStorage(bool optional_create = true);
  ~StorageNamespaceController();

  bool CanAccessStorage(LocalFrame*, StorageArea::StorageType) const;

  static void ProvideStorageNamespaceTo(Page&, WebViewClient*);
  static StorageNamespaceController* From(Page* page) {
    return Supplement<Page>::From<StorageNamespaceController>(page);
  }

  void Trace(blink::Visitor*) override;

  InspectorDOMStorageAgent* InspectorAgent() { return inspector_agent_; }
  void SetInspectorAgent(InspectorDOMStorageAgent* agent) {
    inspector_agent_ = agent;
  }

 private:
  explicit StorageNamespaceController(WebViewClient*);

  std::unique_ptr<StorageNamespace> CreateSessionStorageNamespace();

  std::unique_ptr<StorageNamespace> session_storage_;
  Member<InspectorDOMStorageAgent> inspector_agent_;
  WebViewClient* web_view_client_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_STORAGE_STORAGE_NAMESPACE_CONTROLLER_H_
