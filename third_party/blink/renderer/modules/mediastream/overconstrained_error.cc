// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/mediastream/overconstrained_error.h"

namespace blink {

OverconstrainedError* OverconstrainedError::Create(const String& constraint,
                                                   const String& message) {
  return new OverconstrainedError(constraint, message);
}

OverconstrainedError::OverconstrainedError(const String& constraint,
                                           const String& message)
    : constraint_(constraint), message_(message) {}

}  // namespace blink
