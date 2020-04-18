/*
 * Copyright (C) 2009 Google Inc. All Rights Reserved.
 *           (C) 2008 Apple Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/storage/storage_area.h"

#include <memory>
#include "third_party/blink/public/platform/web_storage_area.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/modules/storage/dom_window_storage.h"
#include "third_party/blink/renderer/modules/storage/inspector_dom_storage_agent.h"
#include "third_party/blink/renderer/modules/storage/storage.h"
#include "third_party/blink/renderer/modules/storage/storage_event.h"
#include "third_party/blink/renderer/modules/storage/storage_namespace.h"
#include "third_party/blink/renderer/modules/storage/storage_namespace_controller.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

StorageArea* StorageArea::Create(std::unique_ptr<WebStorageArea> storage_area,
                                 StorageType storage_type) {
  return new StorageArea(std::move(storage_area), storage_type);
}

StorageArea::StorageArea(std::unique_ptr<WebStorageArea> storage_area,
                         StorageType storage_type)
    : storage_area_(std::move(storage_area)),
      storage_type_(storage_type),
      frame_used_for_can_access_storage_(nullptr),
      can_access_storage_cached_result_(false) {}

StorageArea::~StorageArea() = default;

void StorageArea::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_used_for_can_access_storage_);
}

unsigned StorageArea::length(ExceptionState& exception_state,
                             LocalFrame* frame) {
  if (!CanAccessStorage(frame)) {
    exception_state.ThrowSecurityError("access is denied for this document.");
    return 0;
  }
  return storage_area_->length();
}

String StorageArea::Key(unsigned index,
                        ExceptionState& exception_state,
                        LocalFrame* frame) {
  if (!CanAccessStorage(frame)) {
    exception_state.ThrowSecurityError("access is denied for this document.");
    return String();
  }
  return storage_area_->Key(index);
}

String StorageArea::GetItem(const String& key,
                            ExceptionState& exception_state,
                            LocalFrame* frame) {
  if (!CanAccessStorage(frame)) {
    exception_state.ThrowSecurityError("access is denied for this document.");
    return String();
  }
  return storage_area_->GetItem(key);
}

void StorageArea::SetItem(const String& key,
                          const String& value,
                          ExceptionState& exception_state,
                          LocalFrame* frame) {
  if (!CanAccessStorage(frame)) {
    exception_state.ThrowSecurityError("access is denied for this document.");
    return;
  }
  WebStorageArea::Result result = WebStorageArea::kResultOK;
  storage_area_->SetItem(key, value, frame->GetDocument()->Url(), result);
  if (result != WebStorageArea::kResultOK)
    exception_state.ThrowDOMException(
        kQuotaExceededError,
        "Setting the value of '" + key + "' exceeded the quota.");
}

void StorageArea::RemoveItem(const String& key,
                             ExceptionState& exception_state,
                             LocalFrame* frame) {
  if (!CanAccessStorage(frame)) {
    exception_state.ThrowSecurityError("access is denied for this document.");
    return;
  }
  storage_area_->RemoveItem(key, frame->GetDocument()->Url());
}

void StorageArea::Clear(ExceptionState& exception_state, LocalFrame* frame) {
  if (!CanAccessStorage(frame)) {
    exception_state.ThrowSecurityError("access is denied for this document.");
    return;
  }
  storage_area_->Clear(frame->GetDocument()->Url());
}

bool StorageArea::Contains(const String& key,
                           ExceptionState& exception_state,
                           LocalFrame* frame) {
  if (!CanAccessStorage(frame)) {
    exception_state.ThrowSecurityError("access is denied for this document.");
    return false;
  }
  return !GetItem(key, exception_state, frame).IsNull();
}

bool StorageArea::CanAccessStorage(LocalFrame* frame) {
  if (!frame || !frame->GetPage())
    return false;

  // Should the LocalFrame die before this StorageArea does,
  // that cached reference will be cleared.
  if (frame_used_for_can_access_storage_ == frame)
    return can_access_storage_cached_result_;
  StorageNamespaceController* controller =
      StorageNamespaceController::From(frame->GetPage());
  if (!controller)
    return false;
  bool result = controller->CanAccessStorage(frame, storage_type_);
  // Move attention to the new LocalFrame.
  frame_used_for_can_access_storage_ = frame;
  can_access_storage_cached_result_ = result;
  return result;
}

void StorageArea::DispatchLocalStorageEvent(
    const String& key,
    const String& old_value,
    const String& new_value,
    const SecurityOrigin* security_origin,
    const KURL& page_url,
    WebStorageArea* source_area_instance) {
  // Iterate over all pages that have a StorageNamespaceController supplement.
  for (Page* page : Page::OrdinaryPages()) {
    for (Frame* frame = page->MainFrame(); frame;
         frame = frame->Tree().TraverseNext()) {
      // FIXME: We do not yet have a way to dispatch events to out-of-process
      // frames.
      if (!frame->IsLocalFrame())
        continue;
      LocalFrame* local_frame = ToLocalFrame(frame);
      LocalDOMWindow* local_window = local_frame->DomWindow();
      Storage* storage =
          DOMWindowStorage::From(*local_window).OptionalLocalStorage();
      if (storage &&
          local_frame->GetDocument()->GetSecurityOrigin()->IsSameSchemeHostPort(
              security_origin) &&
          !IsEventSource(storage, source_area_instance))
        local_frame->DomWindow()->EnqueueWindowEvent(
            StorageEvent::Create(EventTypeNames::storage, key, old_value,
                                 new_value, page_url, storage));
    }
    if (InspectorDOMStorageAgent* agent =
            StorageNamespaceController::From(page)->InspectorAgent())
      agent->DidDispatchDOMStorageEvent(key, old_value, new_value,
                                        kLocalStorage, security_origin);
  }
}

static Page* FindPageWithSessionStorageNamespace(
    const WebStorageNamespace& session_namespace) {
  // Iterate over all pages that have a StorageNamespaceController supplement.
  for (Page* page : Page::OrdinaryPages()) {
    const bool kDontCreateIfMissing = false;
    StorageNamespace* storage_namespace =
        StorageNamespaceController::From(page)->SessionStorage(
            kDontCreateIfMissing);
    if (storage_namespace &&
        storage_namespace->IsSameNamespace(session_namespace))
      return page;
  }
  return nullptr;
}

void StorageArea::DispatchSessionStorageEvent(
    const String& key,
    const String& old_value,
    const String& new_value,
    const SecurityOrigin* security_origin,
    const KURL& page_url,
    const WebStorageNamespace& session_namespace,
    WebStorageArea* source_area_instance) {
  Page* page = FindPageWithSessionStorageNamespace(session_namespace);
  if (!page)
    return;

  for (Frame* frame = page->MainFrame(); frame;
       frame = frame->Tree().TraverseNext()) {
    // FIXME: We do not yet have a way to dispatch events to out-of-process
    // frames.
    if (!frame->IsLocalFrame())
      continue;
    LocalFrame* local_frame = ToLocalFrame(frame);
    LocalDOMWindow* local_window = local_frame->DomWindow();
    Storage* storage =
        DOMWindowStorage::From(*local_window).OptionalSessionStorage();
    if (storage &&
        local_frame->GetDocument()->GetSecurityOrigin()->IsSameSchemeHostPort(
            security_origin) &&
        !IsEventSource(storage, source_area_instance))
      local_frame->DomWindow()->EnqueueWindowEvent(
          StorageEvent::Create(EventTypeNames::storage, key, old_value,
                               new_value, page_url, storage));
  }
  if (InspectorDOMStorageAgent* agent =
          StorageNamespaceController::From(page)->InspectorAgent())
    agent->DidDispatchDOMStorageEvent(key, old_value, new_value,
                                      kSessionStorage, security_origin);
}

bool StorageArea::IsEventSource(Storage* storage,
                                WebStorageArea* source_area_instance) {
  DCHECK(storage);
  StorageArea* area = storage->Area();
  return area->storage_area_.get() == source_area_instance;
}

}  // namespace blink
