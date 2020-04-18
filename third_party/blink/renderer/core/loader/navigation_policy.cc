/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/loader/navigation_policy.h"

#include "build/build_config.h"
#include "third_party/blink/public/web/web_navigation_policy.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

bool NavigationPolicyFromMouseEvent(unsigned short button,
                                    bool ctrl,
                                    bool shift,
                                    bool alt,
                                    bool meta,
                                    NavigationPolicy* policy) {
#if defined(OS_MACOSX)
  const bool new_tab_modifier = (button == 1) || meta;
#else
  const bool new_tab_modifier = (button == 1) || ctrl;
#endif
  if (!new_tab_modifier && !shift && !alt)
    return false;

  DCHECK(policy);
  if (new_tab_modifier) {
    if (shift)
      *policy = kNavigationPolicyNewForegroundTab;
    else
      *policy = kNavigationPolicyNewBackgroundTab;
  } else {
    if (shift)
      *policy = kNavigationPolicyNewWindow;
    else
      *policy = kNavigationPolicyDownload;
  }
  return true;
}

STATIC_ASSERT_ENUM(kWebNavigationPolicyIgnore, kNavigationPolicyIgnore);
STATIC_ASSERT_ENUM(kWebNavigationPolicyDownload, kNavigationPolicyDownload);
STATIC_ASSERT_ENUM(kWebNavigationPolicyCurrentTab, kNavigationPolicyCurrentTab);
STATIC_ASSERT_ENUM(kWebNavigationPolicyNewBackgroundTab,
                   kNavigationPolicyNewBackgroundTab);
STATIC_ASSERT_ENUM(kWebNavigationPolicyNewForegroundTab,
                   kNavigationPolicyNewForegroundTab);
STATIC_ASSERT_ENUM(kWebNavigationPolicyNewWindow, kNavigationPolicyNewWindow);
STATIC_ASSERT_ENUM(kWebNavigationPolicyNewPopup, kNavigationPolicyNewPopup);

}  // namespace blink
