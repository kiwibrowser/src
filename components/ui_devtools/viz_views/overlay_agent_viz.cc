// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ui_devtools/viz_views/overlay_agent_viz.h"

namespace ui_devtools {

OverlayAgentViz::OverlayAgentViz(DOMAgentViz* dom_agent)
    : OverlayAgent(dom_agent) {}

OverlayAgentViz::~OverlayAgentViz() {}

protocol::Response OverlayAgentViz::setInspectMode(
    const String& in_mode,
    protocol::Maybe<protocol::Overlay::HighlightConfig> in_highlightConfig) {
  return protocol::Response::OK();
}

protocol::Response OverlayAgentViz::highlightNode(
    std::unique_ptr<protocol::Overlay::HighlightConfig> highlight_config,
    protocol::Maybe<int> node_id) {
  return protocol::Response::OK();
}

protocol::Response OverlayAgentViz::hideHighlight() {
  return protocol::Response::OK();
}

}  // namespace ui_devtools
