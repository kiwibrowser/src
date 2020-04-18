// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/text/hyphenation.h"

namespace blink {

scoped_refptr<Hyphenation> Hyphenation::PlatformGetHyphenation(
    const AtomicString&) {
  // TODO(fuchsia): Implement this when UI support is ready. crbug.com/750946
  return nullptr;
}

}  // namespace blink
