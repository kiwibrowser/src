// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/exported/web_form_element_observer_impl.h"

#include "third_party/blink/public/web/modules/autofill/web_form_element_observer_callback.h"
#include "third_party/blink/public/web/web_form_control_element.h"
#include "third_party/blink/public/web/web_form_element.h"
#include "third_party/blink/renderer/core/css/css_computed_style_declaration.h"
#include "third_party/blink/renderer/core/dom/mutation_observer.h"
#include "third_party/blink/renderer/core/dom/mutation_observer_init.h"
#include "third_party/blink/renderer/core/dom/mutation_record.h"
#include "third_party/blink/renderer/core/dom/static_node_list.h"
#include "third_party/blink/renderer/core/html/forms/html_form_element.h"
#include "third_party/blink/renderer/core/html/html_element.h"

namespace blink {

class WebFormElementObserverImpl::ObserverCallback
    : public MutationObserver::Delegate {
 public:
  ObserverCallback(HTMLElement&,
                   std::unique_ptr<WebFormElementObserverCallback>);

  ExecutionContext* GetExecutionContext() const override;

  void Deliver(const MutationRecordVector& records, MutationObserver&) override;

  void Disconnect();

  void Trace(blink::Visitor*) override;

 private:
  Member<HTMLElement> element_;
  HeapHashSet<Member<Node>> parents_;
  Member<MutationObserver> mutation_observer_;
  std::unique_ptr<WebFormElementObserverCallback> callback_;
};

WebFormElementObserverImpl::ObserverCallback::ObserverCallback(
    HTMLElement& element,
    std::unique_ptr<WebFormElementObserverCallback> callback)
    : element_(element),
      mutation_observer_(MutationObserver::Create(this)),
      callback_(std::move(callback)) {
  {
    MutationObserverInit init;
    init.setAttributes(true);
    init.setAttributeFilter({"class", "style"});
    mutation_observer_->observe(element_, init, ASSERT_NO_EXCEPTION);
  }
  for (Node* node = element_; node->parentElement();
       node = node->parentElement()) {
    MutationObserverInit init;
    init.setChildList(true);
    init.setAttributes(true);
    init.setAttributeFilter({"class", "style"});
    mutation_observer_->observe(node->parentElement(), init,
                                ASSERT_NO_EXCEPTION);
    parents_.insert(node->parentElement());
  }
}

ExecutionContext*
WebFormElementObserverImpl::ObserverCallback::GetExecutionContext() const {
  return element_ ? &element_->GetDocument() : nullptr;
}

void WebFormElementObserverImpl::ObserverCallback::Deliver(
    const MutationRecordVector& records,
    MutationObserver&) {
  for (const auto& record : records) {
    if (record->type() == "childList") {
      for (unsigned i = 0; i < record->removedNodes()->length(); ++i) {
        Node* removed_node = record->removedNodes()->item(i);
        if (removed_node != element_ &&
            parents_.find(removed_node) == parents_.end()) {
          continue;
        }
        callback_->ElementWasHiddenOrRemoved();
        Disconnect();
        return;
      }
    } else {
      // Either "style" or "class" was modified. Check the computed style.
      HTMLElement& element = *ToHTMLElement(record->target());
      CSSComputedStyleDeclaration* style =
          CSSComputedStyleDeclaration::Create(&element);
      if (style->GetPropertyValue(CSSPropertyDisplay) == "none") {
        callback_->ElementWasHiddenOrRemoved();
        Disconnect();
        return;
      }
    }
  }
}

void WebFormElementObserverImpl::ObserverCallback::Disconnect() {
  mutation_observer_->disconnect();
  callback_.reset();
}

void WebFormElementObserverImpl::ObserverCallback::Trace(
    blink::Visitor* visitor) {
  visitor->Trace(element_);
  visitor->Trace(parents_);
  visitor->Trace(mutation_observer_);
  MutationObserver::Delegate::Trace(visitor);
}

WebFormElementObserver* WebFormElementObserver::Create(
    WebFormElement& element,
    std::unique_ptr<WebFormElementObserverCallback> callback) {
  return new WebFormElementObserverImpl(*element.Unwrap<HTMLFormElement>(),
                                        std::move(callback));
}

WebFormElementObserver* WebFormElementObserver::Create(
    WebFormControlElement& element,
    std::unique_ptr<WebFormElementObserverCallback> callback) {
  return new WebFormElementObserverImpl(*element.Unwrap<HTMLElement>(),
                                        std::move(callback));
}

WebFormElementObserverImpl::WebFormElementObserverImpl(
    HTMLElement& element,
    std::unique_ptr<WebFormElementObserverCallback> callback)
    : self_keep_alive_(this) {
  mutation_callback_ = new ObserverCallback(element, std::move(callback));
}

WebFormElementObserverImpl::~WebFormElementObserverImpl() = default;

void WebFormElementObserverImpl::Disconnect() {
  mutation_callback_->Disconnect();
  mutation_callback_ = nullptr;
  self_keep_alive_.Clear();
}

void WebFormElementObserverImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(mutation_callback_);
}

}  // namespace blink
