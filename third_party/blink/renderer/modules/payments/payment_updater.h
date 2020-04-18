// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_PAYMENT_UPDATER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_PAYMENT_UPDATER_H_

#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ScriptValue;

class MODULES_EXPORT PaymentUpdater : public GarbageCollectedMixin {
 public:
  virtual void OnUpdatePaymentDetails(
      const ScriptValue& details_script_value) = 0;
  virtual void OnUpdatePaymentDetailsFailure(const String& error) = 0;

 protected:
  virtual ~PaymentUpdater() = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_PAYMENT_UPDATER_H_
