// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGTextEndEffect_h
#define NGTextEndEffect_h

namespace blink {

// Effects at the end of text fragments.
enum class NGTextEndEffect {
  kNone,
  kHyphen,

  // When adding new values, ensure NGPhysicalTextFragment has enough bits.
};

}  // namespace blink

#endif  // NGTextEndEffect_h
