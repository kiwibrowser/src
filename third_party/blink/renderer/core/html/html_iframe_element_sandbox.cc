// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/html_iframe_element_sandbox.h"

#include "third_party/blink/renderer/core/html/html_iframe_element.h"

namespace blink {

namespace {

// These are the sandbox tokens which are always supported. If a new token is
// only available behind a runtime flag, it should be checked separately in
// IsTokenSupported below.
const char* const kSupportedSandboxTokens[] = {
    "allow-forms",
    "allow-modals",
    "allow-orientation-lock",
    "allow-pointer-lock",
    "allow-popups",
    "allow-popups-to-escape-sandbox",
    "allow-presentation",
    "allow-same-origin",
    "allow-scripts",
    "allow-top-navigation",
    "allow-top-navigation-by-user-activation"};

bool IsTokenSupported(const AtomicString& token) {
  for (const char* supported_token : kSupportedSandboxTokens) {
    if (token == supported_token)
      return true;
  }
  if (token == "allow-downloads" &&
      RuntimeEnabledFeatures::BlockingDownloadsInSandboxEnabled()) {
    return true;
  }
  return false;
}

}  // namespace

HTMLIFrameElementSandbox::HTMLIFrameElementSandbox(HTMLIFrameElement* element)
    : DOMTokenList(*element, HTMLNames::sandboxAttr) {}

bool HTMLIFrameElementSandbox::ValidateTokenValue(
    const AtomicString& token_value,
    ExceptionState&) const {
  return IsTokenSupported(token_value);
}

}  // namespace blink
