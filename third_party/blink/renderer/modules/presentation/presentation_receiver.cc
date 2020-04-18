// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/presentation/presentation_receiver.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/modules/presentation/navigator_presentation.h"
#include "third_party/blink/renderer/modules/presentation/presentation.h"
#include "third_party/blink/renderer/modules/presentation/presentation_connection.h"
#include "third_party/blink/renderer/modules/presentation/presentation_connection_list.h"

namespace blink {

PresentationReceiver::PresentationReceiver(LocalFrame* frame)
    : ContextLifecycleObserver(frame->GetDocument()),
      connection_list_(new PresentationConnectionList(frame->GetDocument())),
      receiver_binding_(this) {
  auto* interface_provider = GetFrame()->Client()->GetInterfaceProvider();
  interface_provider->GetInterface(mojo::MakeRequest(&presentation_service_));

  mojom::blink::PresentationReceiverPtr receiver_ptr;
  receiver_binding_.Bind(mojo::MakeRequest(&receiver_ptr));
  presentation_service_->SetReceiver(std::move(receiver_ptr));
}

// static
PresentationReceiver* PresentationReceiver::From(Document& document) {
  if (!document.IsInMainFrame() || !document.GetFrame()->DomWindow())
    return nullptr;
  Navigator& navigator = *document.GetFrame()->DomWindow()->navigator();
  Presentation* presentation = NavigatorPresentation::presentation(navigator);
  if (!presentation)
    return nullptr;

  return presentation->receiver();
}

ScriptPromise PresentationReceiver::connectionList(ScriptState* script_state) {
  ExecutionContext* execution_context = ExecutionContext::From(script_state);
  RecordOriginTypeAccess(*execution_context);
  if (!connection_list_property_) {
    connection_list_property_ = new ConnectionListProperty(
        execution_context, this, ConnectionListProperty::kReady);
  }

  if (!connection_list_->IsEmpty() && connection_list_property_->GetState() ==
                                          ScriptPromisePropertyBase::kPending)
    connection_list_property_->Resolve(connection_list_);

  return connection_list_property_->Promise(script_state->World());
}

void PresentationReceiver::Terminate() {
  if (!GetFrame())
    return;

  auto* window = GetFrame()->DomWindow();
  if (!window || window->closed())
    return;

  window->close(window);
}

void PresentationReceiver::RemoveConnection(
    ReceiverPresentationConnection* connection) {
  DCHECK(connection_list_);
  connection_list_->RemoveConnection(connection);
}

void PresentationReceiver::OnReceiverConnectionAvailable(
    mojom::blink::PresentationInfoPtr info,
    mojom::blink::PresentationConnectionPtr controller_connection,
    mojom::blink::PresentationConnectionRequest receiver_connection_request) {
  // Take() will call PresentationReceiver::registerConnection()
  // and register the connection.
  auto* connection = ReceiverPresentationConnection::Take(
      this, *info, std::move(controller_connection),
      std::move(receiver_connection_request));

  // Only notify receiver.connectionList property if it has been acccessed
  // previously.
  if (!connection_list_property_)
    return;

  if (connection_list_property_->GetState() ==
      ScriptPromisePropertyBase::kPending) {
    connection_list_property_->Resolve(connection_list_);
  } else if (connection_list_property_->GetState() ==
             ScriptPromisePropertyBase::kResolved) {
    connection_list_->DispatchConnectionAvailableEvent(connection);
  }
}

void PresentationReceiver::RegisterConnection(
    ReceiverPresentationConnection* connection) {
  DCHECK(connection_list_);
  connection_list_->AddConnection(connection);
}

// static
void PresentationReceiver::RecordOriginTypeAccess(
    ExecutionContext& execution_context) {
  if (execution_context.IsSecureContext()) {
    UseCounter::Count(&execution_context,
                      WebFeature::kPresentationReceiverSecureOrigin);
  } else {
    Deprecation::CountDeprecation(
        &execution_context, WebFeature::kPresentationReceiverInsecureOrigin);
  }
}

void PresentationReceiver::ContextDestroyed(ExecutionContext*) {
  receiver_binding_.Close();
  presentation_service_.reset();
}

void PresentationReceiver::Trace(blink::Visitor* visitor) {
  visitor->Trace(connection_list_);
  visitor->Trace(connection_list_property_);
  ScriptWrappable::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
