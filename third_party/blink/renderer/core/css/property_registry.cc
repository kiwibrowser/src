// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/property_registry.h"

namespace blink {

void PropertyRegistry::RegisterProperty(const AtomicString& name,
                                        PropertyRegistration& registration) {
  DCHECK(!Registration(name));
  registrations_.Set(name, &registration);
}

const PropertyRegistration* PropertyRegistry::Registration(
    const AtomicString& name) const {
  return registrations_.at(name);
}

}  // namespace blink
