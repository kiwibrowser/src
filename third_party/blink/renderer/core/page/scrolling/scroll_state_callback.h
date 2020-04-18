// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_SCROLL_STATE_CALLBACK_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_SCROLL_STATE_CALLBACK_H_

#include "third_party/blink/public/platform/web_native_scroll_behavior.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_scroll_state_callback.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class ScrollState;

class ScrollStateCallback
    : public GarbageCollectedFinalized<ScrollStateCallback> {
 public:
  virtual ~ScrollStateCallback() = default;

  virtual void Trace(blink::Visitor* visitor) {}

  virtual void Invoke(ScrollState*) = 0;

  WebNativeScrollBehavior NativeScrollBehavior() const {
    return native_scroll_behavior_;
  }

 protected:
  explicit ScrollStateCallback(
      WebNativeScrollBehavior native_scroll_behavior =
          WebNativeScrollBehavior::kDisableNativeScroll)
      : native_scroll_behavior_(native_scroll_behavior) {}

 private:
  const WebNativeScrollBehavior native_scroll_behavior_;
};

class ScrollStateCallbackV8Impl : public ScrollStateCallback {
 public:
  static ScrollStateCallbackV8Impl* Create(
      V8ScrollStateCallback* callback,
      const String& native_scroll_behavior) {
    DCHECK(callback);
    return new ScrollStateCallbackV8Impl(
        callback, ParseNativeScrollBehavior(native_scroll_behavior));
  }

  ~ScrollStateCallbackV8Impl() override = default;

  void Trace(blink::Visitor*) override;

  void Invoke(ScrollState*) override;

 private:
  static WebNativeScrollBehavior ParseNativeScrollBehavior(
      const String& native_scroll_behavior);

  explicit ScrollStateCallbackV8Impl(
      V8ScrollStateCallback* callback,
      WebNativeScrollBehavior native_scroll_behavior)
      : ScrollStateCallback(native_scroll_behavior),
        callback_(ToV8PersistentCallbackFunction(callback)) {}

  Member<V8PersistentCallbackFunction<V8ScrollStateCallback>> callback_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_SCROLL_STATE_CALLBACK_H_
