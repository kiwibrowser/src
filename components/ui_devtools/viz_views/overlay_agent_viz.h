// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_VIZ_VIEWS_OVERLAY_AGENT_VIZ_H_
#define COMPONENTS_UI_DEVTOOLS_VIZ_VIEWS_OVERLAY_AGENT_VIZ_H_

#include "components/ui_devtools/Overlay.h"
#include "components/ui_devtools/overlay_agent.h"
#include "components/ui_devtools/viz_views/dom_agent_viz.h"

namespace ui_devtools {

class OverlayAgentViz : public OverlayAgent {
 public:
  explicit OverlayAgentViz(DOMAgentViz* dom_agent);
  ~OverlayAgentViz() override;

  // Overlay::Backend:
  protocol::Response setInspectMode(
      const String& in_mode,
      protocol::Maybe<protocol::Overlay::HighlightConfig> in_highlightConfig)
      override;
  protocol::Response highlightNode(
      std::unique_ptr<protocol::Overlay::HighlightConfig> highlight_config,
      protocol::Maybe<int> node_id) override;
  protocol::Response hideHighlight() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(OverlayAgentViz);
};

}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_VIZ_VIEWS_OVERLAY_AGENT_VIZ_H_
