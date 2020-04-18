// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_FRAME_OWNER_PROPERTIES_H_
#define CONTENT_COMMON_FRAME_OWNER_PROPERTIES_H_

#include <vector>

#include "content/common/content_export.h"
#include "third_party/blink/public/common/feature_policy/feature_policy.h"
#include "third_party/blink/public/web/web_frame_owner_properties.h"

namespace content {

// Used for IPC transport of WebFrameOwnerProperties. WebFrameOwnerProperties
// can't be used directly as it contains a WebVector which doesn't have
// ParamTraits defined.
struct CONTENT_EXPORT FrameOwnerProperties {
  FrameOwnerProperties();
  FrameOwnerProperties(const FrameOwnerProperties& other);
  ~FrameOwnerProperties();

  bool operator==(const FrameOwnerProperties& other) const;
  bool operator!=(const FrameOwnerProperties& other) const {
    return !(*this == other);
  }

  std::string name;  // browsing context container's name
  blink::WebFrameOwnerProperties::ScrollingMode scrolling_mode;
  int margin_width;
  int margin_height;
  bool allow_fullscreen;
  bool allow_payment_request;
  bool is_display_none;

  // An experimental attribute to be used by a parent frame to enforce CSP on a
  // subframe. This is different from replicated CSP headers kept in
  // FrameReplicationState that keep track of CSP headers currently in effect
  // for a frame. See https://crbug.com/647588 and
  // https://www.w3.org/TR/csp-embedded-enforcement/#required-csp
  std::string required_csp;
};

}  // namespace content

#endif  // CONTENT_COMMON_FRAME_OWNER_PROPERTIES_H_
