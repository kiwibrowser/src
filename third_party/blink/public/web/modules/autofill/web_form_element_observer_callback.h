// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_AUTOFILL_WEB_FORM_ELEMENT_OBSERVER_CALLBACK_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_AUTOFILL_WEB_FORM_ELEMENT_OBSERVER_CALLBACK_H_

namespace blink {

class WebFormElementObserverCallback {
 public:
  virtual ~WebFormElementObserverCallback() = default;

  // Invoked when the observed element was either removed from the DOM or it's
  // computed style changed to display: none.
  virtual void ElementWasHiddenOrRemoved() = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_AUTOFILL_WEB_FORM_ELEMENT_OBSERVER_CALLBACK_H_
