// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_TOOLBAR_STATE_H_
#define CHROME_BROWSER_VR_MODEL_TOOLBAR_STATE_H_

#include "components/security_state/core/security_state.h"
#include "url/gurl.h"

namespace gfx {
struct VectorIcon;
}

namespace vr {

// Passes information obtained from ToolbarModel to the VR UI framework.
struct ToolbarState {
 public:
  ToolbarState();
  ToolbarState(const GURL& url,
               security_state::SecurityLevel level,
               const gfx::VectorIcon* icon,
               bool display_url,
               bool offline);
  ToolbarState(const ToolbarState& other);

  bool operator==(const ToolbarState& other) const;
  bool operator!=(const ToolbarState& other) const;

  GURL gurl;
  security_state::SecurityLevel security_level;
  const gfx::VectorIcon* vector_icon;
  bool should_display_url;
  bool offline_page;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_TOOLBAR_STATE_H_
