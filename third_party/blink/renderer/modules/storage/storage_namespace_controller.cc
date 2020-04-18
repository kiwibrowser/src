// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/storage/storage_namespace_controller.h"

#include <memory>

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_storage_namespace.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/core/frame/content_settings_client.h"
#include "third_party/blink/renderer/modules/storage/inspector_dom_storage_agent.h"
#include "third_party/blink/renderer/modules/storage/storage_namespace.h"

namespace blink {

#define STATIC_ASSERT_MATCHING_ENUM(enum_name1, enum_name2)                   \
  static_assert(static_cast<int>(enum_name1) == static_cast<int>(enum_name2), \
                "mismatching enums: " #enum_name1)
STATIC_ASSERT_MATCHING_ENUM(StorageArea::kLocalStorage,
                            ContentSettingsClient::StorageType::kLocal);
STATIC_ASSERT_MATCHING_ENUM(StorageArea::kSessionStorage,
                            ContentSettingsClient::StorageType::kSession);

const char StorageNamespaceController::kSupplementName[] =
    "StorageNamespaceController";

StorageNamespaceController::StorageNamespaceController(WebViewClient* client)
    : inspector_agent_(nullptr), web_view_client_(client) {}

StorageNamespaceController::~StorageNamespaceController() = default;

void StorageNamespaceController::Trace(blink::Visitor* visitor) {
  Supplement<Page>::Trace(visitor);
  visitor->Trace(inspector_agent_);
}

StorageNamespace* StorageNamespaceController::SessionStorage(
    bool optional_create) {
  if (!session_storage_ && optional_create)
    session_storage_ = CreateSessionStorageNamespace();
  return session_storage_.get();
}

void StorageNamespaceController::ProvideStorageNamespaceTo(
    Page& page,
    WebViewClient* client) {
  ProvideTo(page, new StorageNamespaceController(client));
}

std::unique_ptr<StorageNamespace>
StorageNamespaceController::CreateSessionStorageNamespace() {
  if (!web_view_client_)
    return nullptr;

  return std::make_unique<StorageNamespace>(
      Platform::Current()->CreateSessionStorageNamespace(
          web_view_client_->GetSessionStorageNamespaceId()));
}

bool StorageNamespaceController::CanAccessStorage(
    LocalFrame* frame,
    StorageArea::StorageType type) const {
  DCHECK(frame->GetContentSettingsClient());
  return frame->GetContentSettingsClient()->AllowStorage(
      static_cast<ContentSettingsClient::StorageType>(type));
}

}  // namespace blink
