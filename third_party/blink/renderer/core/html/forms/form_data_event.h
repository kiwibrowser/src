// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_FORM_DATA_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_FORM_DATA_EVENT_H_

#include "third_party/blink/renderer/core/dom/events/event.h"

namespace blink {

class FormData;

class FormDataEvent : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static FormDataEvent* Create(FormData& form_data);
  void Trace(Visitor* visitor) override;

  FormData* formData() const { return form_data_; };

  const AtomicString& InterfaceName() const override;

 private:
  FormDataEvent(FormData& form_data);

  Member<FormData> form_data_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_FORM_DATA_EVENT_H_
