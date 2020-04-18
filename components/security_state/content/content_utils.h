// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SECURITY_STATE_CONTENT_CONTENT_UTILS_H_
#define COMPONENTS_SECURITY_STATE_CONTENT_CONTENT_UTILS_H_

#include <memory>

#include "third_party/blink/public/platform/web_security_style.h"

namespace content {
struct SecurityStyleExplanations;
class WebContents;
}  // namespace content

namespace security_state {
struct SecurityInfo;
struct VisibleSecurityState;
}  // namespace security_state

namespace security_state {

// Retrieves the visible security state that is relevant to GetSecurityInfo()
// from the current page in |web_contents|.
std::unique_ptr<security_state::VisibleSecurityState> GetVisibleSecurityState(
    content::WebContents* web_contents);

// Returns the SecurityStyle that should be applied to a WebContents
// with the given |security_info|. Populates
// |security_style_explanations| to explain why the returned
// SecurityStyle was chosen.
blink::WebSecurityStyle GetSecurityStyle(
    const security_state::SecurityInfo& security_info,
    content::SecurityStyleExplanations* security_style_explanations);

}  // namespace security_state

#endif  // COMPONENTS_SECURITY_STATE_CONTENT_CONTENT_UTILS_H_
