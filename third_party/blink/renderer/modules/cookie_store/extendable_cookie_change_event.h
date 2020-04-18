// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_EXTENDABLE_COOKIE_CHANGE_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_EXTENDABLE_COOKIE_CHANGE_EVENT_H_

#include "third_party/blink/renderer/modules/cookie_store/cookie_list_item.h"
#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/serviceworkers/extendable_event.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ExtendableCookieChangeEventInit;
class WaitUntilObserver;
class WebString;

class ExtendableCookieChangeEvent final : public ExtendableEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // Used by Blink.
  //
  // The caller is expected to create HeapVectors and std::move() them into this
  // method.
  static ExtendableCookieChangeEvent* Create(
      const AtomicString& type,
      HeapVector<CookieListItem> changed,
      HeapVector<CookieListItem> deleted,
      WaitUntilObserver* wait_until_observer) {
    return new ExtendableCookieChangeEvent(
        type, std::move(changed), std::move(deleted), wait_until_observer);
  }

  // Used by JavaScript, via the V8 bindings.
  static ExtendableCookieChangeEvent* Create(
      const AtomicString& type,
      const ExtendableCookieChangeEventInit& initializer) {
    return new ExtendableCookieChangeEvent(type, initializer);
  }

  ~ExtendableCookieChangeEvent() override;

  const HeapVector<CookieListItem>& changed() const { return changed_; }
  const HeapVector<CookieListItem>& deleted() const { return deleted_; }

  // Event
  const AtomicString& InterfaceName() const override;

  // GarbageCollected
  void Trace(blink::Visitor*) override;

  // Helper for converting backend event information into a CookieChangeEvent.
  //
  // TODO(pwnall): Switch to blink::CanonicalCookie when
  //               https://crrev.com/c/991196 lands.
  static void ToCookieChangeListItem(const WebString& cookie_name,
                                     const WebString& cookie_value,
                                     bool is_cookie_delete,
                                     HeapVector<CookieListItem>& changed,
                                     HeapVector<CookieListItem>& deleted);

 private:
  ExtendableCookieChangeEvent(const AtomicString& type,
                              HeapVector<CookieListItem> changed,
                              HeapVector<CookieListItem> deleted,
                              WaitUntilObserver*);
  ExtendableCookieChangeEvent(
      const AtomicString& type,
      const ExtendableCookieChangeEventInit& initializer);

  HeapVector<CookieListItem> changed_;
  HeapVector<CookieListItem> deleted_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_EXTENDABLE_COOKIE_CHANGE_EVENT_H_
