// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_INSTALL_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_INSTALL_EVENT_H_

#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/serviceworkers/extendable_event.h"

namespace blink {

class MODULES_EXPORT InstallEvent : public ExtendableEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static InstallEvent* Create(const AtomicString& type,
                              const ExtendableEventInit&);
  static InstallEvent* Create(const AtomicString& type,
                              const ExtendableEventInit&,
                              int event_id,
                              WaitUntilObserver*);

  ~InstallEvent() override;

  const AtomicString& InterfaceName() const override;

 protected:
  InstallEvent(const AtomicString& type, const ExtendableEventInit&);
  InstallEvent(const AtomicString& type,
               const ExtendableEventInit&,
               int event_id,
               WaitUntilObserver*);
  const int event_id_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_INSTALL_EVENT_H_
