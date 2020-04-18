/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/storage/inspector_dom_storage_agent.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/modules/storage/storage.h"
#include "third_party/blink/renderer/modules/storage/storage_namespace.h"
#include "third_party/blink/renderer/modules/storage/storage_namespace_controller.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

using protocol::Response;

namespace DOMStorageAgentState {
static const char kDomStorageAgentEnabled[] = "domStorageAgentEnabled";
};

static Response ToResponse(ExceptionState& exception_state) {
  if (!exception_state.HadException())
    return Response::OK();
  return Response::Error(DOMException::GetErrorName(exception_state.Code()) +
                         " " + exception_state.Message());
}

InspectorDOMStorageAgent::InspectorDOMStorageAgent(
    InspectedFrames* inspected_frames)
    : inspected_frames_(inspected_frames), is_enabled_(false) {}

InspectorDOMStorageAgent::~InspectorDOMStorageAgent() = default;

void InspectorDOMStorageAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(inspected_frames_);
  InspectorBaseAgent::Trace(visitor);
}

void InspectorDOMStorageAgent::Restore() {
  if (state_->booleanProperty(DOMStorageAgentState::kDomStorageAgentEnabled,
                              false)) {
    enable();
  }
}

Response InspectorDOMStorageAgent::enable() {
  if (is_enabled_)
    return Response::OK();
  is_enabled_ = true;
  state_->setBoolean(DOMStorageAgentState::kDomStorageAgentEnabled, true);
  if (StorageNamespaceController* controller = StorageNamespaceController::From(
          inspected_frames_->Root()->GetPage()))
    controller->SetInspectorAgent(this);
  return Response::OK();
}

Response InspectorDOMStorageAgent::disable() {
  if (!is_enabled_)
    return Response::OK();
  is_enabled_ = false;
  state_->setBoolean(DOMStorageAgentState::kDomStorageAgentEnabled, false);
  if (StorageNamespaceController* controller = StorageNamespaceController::From(
          inspected_frames_->Root()->GetPage()))
    controller->SetInspectorAgent(nullptr);
  return Response::OK();
}

Response InspectorDOMStorageAgent::clear(
    std::unique_ptr<protocol::DOMStorage::StorageId> storage_id) {
  LocalFrame* frame = nullptr;
  StorageArea* storage_area = nullptr;
  Response response =
      FindStorageArea(std::move(storage_id), frame, storage_area);
  if (!response.isSuccess())
    return response;
  DummyExceptionStateForTesting exception_state;
  storage_area->Clear(exception_state, frame);
  if (exception_state.HadException())
    return Response::Error("Could not clear the storage");
  return Response::OK();
}

Response InspectorDOMStorageAgent::getDOMStorageItems(
    std::unique_ptr<protocol::DOMStorage::StorageId> storage_id,
    std::unique_ptr<protocol::Array<protocol::Array<String>>>* items) {
  LocalFrame* frame = nullptr;
  StorageArea* storage_area = nullptr;
  Response response =
      FindStorageArea(std::move(storage_id), frame, storage_area);
  if (!response.isSuccess())
    return response;

  std::unique_ptr<protocol::Array<protocol::Array<String>>> storage_items =
      protocol::Array<protocol::Array<String>>::create();

  DummyExceptionStateForTesting exception_state;
  for (unsigned i = 0; i < storage_area->length(exception_state, frame); ++i) {
    String name(storage_area->Key(i, exception_state, frame));
    response = ToResponse(exception_state);
    if (!response.isSuccess())
      return response;
    String value(storage_area->GetItem(name, exception_state, frame));
    response = ToResponse(exception_state);
    if (!response.isSuccess())
      return response;
    std::unique_ptr<protocol::Array<String>> entry =
        protocol::Array<String>::create();
    entry->addItem(name);
    entry->addItem(value);
    storage_items->addItem(std::move(entry));
  }
  *items = std::move(storage_items);
  return Response::OK();
}

Response InspectorDOMStorageAgent::setDOMStorageItem(
    std::unique_ptr<protocol::DOMStorage::StorageId> storage_id,
    const String& key,
    const String& value) {
  LocalFrame* frame = nullptr;
  StorageArea* storage_area = nullptr;
  Response response =
      FindStorageArea(std::move(storage_id), frame, storage_area);
  if (!response.isSuccess())
    return response;

  DummyExceptionStateForTesting exception_state;
  storage_area->SetItem(key, value, exception_state, frame);
  return ToResponse(exception_state);
}

Response InspectorDOMStorageAgent::removeDOMStorageItem(
    std::unique_ptr<protocol::DOMStorage::StorageId> storage_id,
    const String& key) {
  LocalFrame* frame = nullptr;
  StorageArea* storage_area = nullptr;
  Response response =
      FindStorageArea(std::move(storage_id), frame, storage_area);
  if (!response.isSuccess())
    return response;

  DummyExceptionStateForTesting exception_state;
  storage_area->RemoveItem(key, exception_state, frame);
  return ToResponse(exception_state);
}

std::unique_ptr<protocol::DOMStorage::StorageId>
InspectorDOMStorageAgent::GetStorageId(const SecurityOrigin* security_origin,
                                       bool is_local_storage) {
  return protocol::DOMStorage::StorageId::create()
      .setSecurityOrigin(security_origin->ToRawString())
      .setIsLocalStorage(is_local_storage)
      .build();
}

void InspectorDOMStorageAgent::DidDispatchDOMStorageEvent(
    const String& key,
    const String& old_value,
    const String& new_value,
    StorageArea::StorageType storage_type,
    const SecurityOrigin* security_origin) {
  if (!GetFrontend())
    return;

  std::unique_ptr<protocol::DOMStorage::StorageId> id =
      GetStorageId(security_origin, storage_type == StorageArea::kLocalStorage);

  if (key.IsNull())
    GetFrontend()->domStorageItemsCleared(std::move(id));
  else if (new_value.IsNull())
    GetFrontend()->domStorageItemRemoved(std::move(id), key);
  else if (old_value.IsNull())
    GetFrontend()->domStorageItemAdded(std::move(id), key, new_value);
  else
    GetFrontend()->domStorageItemUpdated(std::move(id), key, old_value,
                                         new_value);
}

Response InspectorDOMStorageAgent::FindStorageArea(
    std::unique_ptr<protocol::DOMStorage::StorageId> storage_id,
    LocalFrame*& frame,
    StorageArea*& storage_area) {
  String security_origin = storage_id->getSecurityOrigin();
  bool is_local_storage = storage_id->getIsLocalStorage();
  frame =
      inspected_frames_->FrameWithSecurityOrigin(security_origin);
  if (!frame)
    return Response::Error("Frame not found for the given security origin");

  if (is_local_storage) {
    if (!frame->GetDocument()->GetSecurityOrigin()->CanAccessLocalStorage())
      return Response::Error("Security origin cannot access local storage");
    storage_area = StorageNamespace::LocalStorageArea(
        frame->GetDocument()->GetSecurityOrigin());
    return Response::OK();
  }

  if (!frame->GetDocument()->GetSecurityOrigin()->CanAccessSessionStorage())
    return Response::Error("Security origin cannot access session storage");
  StorageNamespace* session_storage =
      StorageNamespaceController::From(frame->GetPage())->SessionStorage();
  if (!session_storage)
    return Response::Error("SessionStorage is not supported");
  storage_area = session_storage->GetStorageArea(
      frame->GetDocument()->GetSecurityOrigin());
  return Response::OK();
}

}  // namespace blink
