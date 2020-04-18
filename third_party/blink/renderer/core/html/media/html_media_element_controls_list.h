// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_MEDIA_ELEMENT_CONTROLS_LIST_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_MEDIA_ELEMENT_CONTROLS_LIST_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/dom_token_list.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class HTMLMediaElement;

class HTMLMediaElementControlsList final : public DOMTokenList {
 public:
  static HTMLMediaElementControlsList* Create(HTMLMediaElement* element) {
    return new HTMLMediaElementControlsList(element);
  }

  // Whether the list dictates to hide a certain control.
  CORE_EXPORT bool ShouldHideDownload() const;
  CORE_EXPORT bool ShouldHideFullscreen() const;
  CORE_EXPORT bool ShouldHideRemotePlayback() const;

 private:
  explicit HTMLMediaElementControlsList(HTMLMediaElement*);
  bool ValidateTokenValue(const AtomicString&, ExceptionState&) const override;
};

}  // namespace blink

#endif
