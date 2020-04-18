/*
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_SANDBOX_FLAGS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_SANDBOX_FLAGS_H_

#include "third_party/blink/renderer/core/dom/space_split_string.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

enum SandboxFlag {
  // See http://www.whatwg.org/specs/web-apps/current-work/#attr-iframe-sandbox
  // for a list of the sandbox flags.
  kSandboxNone = 0,
  kSandboxNavigation = 1,
  kSandboxPlugins = 1 << 1,
  kSandboxOrigin = 1 << 2,
  kSandboxForms = 1 << 3,
  kSandboxScripts = 1 << 4,
  kSandboxTopNavigation = 1 << 5,
  // See https://www.w3.org/Bugs/Public/show_bug.cgi?id=12393
  kSandboxPopups = 1 << 6,
  kSandboxAutomaticFeatures = 1 << 7,
  kSandboxPointerLock = 1 << 8,
  kSandboxDocumentDomain = 1 << 9,
  // See
  // https://w3c.github.io/screen-orientation/#dfn-sandboxed-orientation-lock-browsing-context-flag.
  kSandboxOrientationLock = 1 << 10,
  kSandboxPropagatesToAuxiliaryBrowsingContexts = 1 << 11,
  kSandboxModals = 1 << 12,
  // See
  // https://w3c.github.io/presentation-api/#sandboxing-and-the-allow-presentation-keyword
  kSandboxPresentationController = 1 << 13,
  // See https://github.com/WICG/interventions/issues/42.
  kSandboxTopNavigationByUserActivation = 1 << 14,
  // See https://crbug.com/539938
  kSandboxDownloads = 1 << 15,
  kSandboxAll = -1  // Mask with all bits set to 1.
};

typedef int SandboxFlags;

SandboxFlags ParseSandboxPolicy(const SpaceSplitString& policy,
                                String& invalid_tokens_error_message);

}  // namespace blink

#endif
