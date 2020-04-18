// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_COOKIE_CHANGE_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_COOKIE_CHANGE_EVENT_H_

#include "third_party/blink/renderer/modules/cookie_store/cookie_list_item.h"
#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class CookieChangeEventInit;

class CookieChangeEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static CookieChangeEvent* Create() { return new CookieChangeEvent(); }

  // Used by Blink.
  //
  // The caller is expected to create HeapVectors and std::move() them into this
  // method.
  static CookieChangeEvent* Create(const AtomicString& type,
                                   HeapVector<CookieListItem> changed,
                                   HeapVector<CookieListItem> deleted) {
    return new CookieChangeEvent(type, std::move(changed), std::move(deleted));
  }

  // Used by JavaScript, via the V8 bindings.
  static CookieChangeEvent* Create(const AtomicString& type,
                                   const CookieChangeEventInit& initializer) {
    return new CookieChangeEvent(type, initializer);
  }

  ~CookieChangeEvent() override;

  const HeapVector<CookieListItem>& changed() const { return changed_; }
  const HeapVector<CookieListItem>& deleted() const { return deleted_; }

  // Event
  const AtomicString& InterfaceName() const override;

  // GarbageCollected
  void Trace(blink::Visitor*) override;

 private:
  CookieChangeEvent();
  CookieChangeEvent(const AtomicString& type,
                    HeapVector<CookieListItem> changed,
                    HeapVector<CookieListItem> deleted);
  CookieChangeEvent(const AtomicString& type,
                    const CookieChangeEventInit& initializer);

  HeapVector<CookieListItem> changed_;
  HeapVector<CookieListItem> deleted_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COOKIE_STORE_COOKIE_CHANGE_EVENT_H_
