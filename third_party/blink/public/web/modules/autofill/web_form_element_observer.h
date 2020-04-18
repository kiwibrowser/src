// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_AUTOFILL_WEB_FORM_ELEMENT_OBSERVER_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_AUTOFILL_WEB_FORM_ELEMENT_OBSERVER_H_

#include <memory>

#include "third_party/blink/public/platform/web_common.h"

namespace blink {

class WebFormControlElement;
class WebFormElement;
class WebFormElementObserverCallback;

class BLINK_EXPORT WebFormElementObserver {
 public:
  // Creates a WebFormElementObserver. Delete this WebFormElementObsrver by
  // calling WebFormElementObserver::Disconnect.
  static WebFormElementObserver* Create(
      WebFormElement&,
      std::unique_ptr<WebFormElementObserverCallback>);
  static WebFormElementObserver* Create(
      WebFormControlElement&,
      std::unique_ptr<WebFormElementObserverCallback>);

  virtual void Disconnect() = 0;

 protected:
  WebFormElementObserver() = default;
  virtual ~WebFormElementObserver() = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_AUTOFILL_WEB_FORM_ELEMENT_OBSERVER_H_
