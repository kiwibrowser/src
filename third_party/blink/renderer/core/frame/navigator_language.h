// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_NAVIGATOR_LANGUAGE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_NAVIGATOR_LANGUAGE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class CORE_EXPORT NavigatorLanguage {
 public:
  NavigatorLanguage();

  AtomicString language();
  virtual Vector<String> languages() = 0;
  bool hasLanguagesChanged() const;
  void SetLanguagesChanged();

 protected:
  bool languages_changed_ = true;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_NAVIGATOR_LANGUAGE_H_
