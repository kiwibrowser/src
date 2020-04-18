// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_VIZ_VIEWS_FRAME_SINK_ELEMENT_H_
#define COMPONENTS_UI_DEVTOOLS_VIZ_VIEWS_FRAME_SINK_ELEMENT_H_

#include "base/macros.h"
#include "components/ui_devtools/ui_element.h"
#include "components/viz/common/surfaces/frame_sink_id.h"

namespace viz {
class FrameSinkManagerImpl;
}

namespace ui_devtools {

class FrameSinkElement : public UIElement {
 public:
  FrameSinkElement(const viz::FrameSinkId& frame_sink_id,
                   viz::FrameSinkManagerImpl* frame_sink_manager,
                   UIElementDelegate* ui_element_delegate,
                   UIElement* parent,
                   bool is_root,
                   bool is_registered,
                   bool is_client_connected);
  ~FrameSinkElement() override;

  // UIElement:
  std::vector<std::pair<std::string, std::string>> GetCustomProperties()
      const override;
  void GetBounds(gfx::Rect* bounds) const override;
  void SetBounds(const gfx::Rect& bounds) override;
  void GetVisible(bool* visible) const override;
  void SetVisible(bool visible) override;
  std::unique_ptr<protocol::Array<std::string>> GetAttributes() const override;
  std::pair<gfx::NativeWindow, gfx::Rect> GetNodeWindowAndBounds()
      const override;

  static const viz::FrameSinkId& From(const UIElement* element);

 private:
  const viz::FrameSinkId frame_sink_id_;
  viz::FrameSinkManagerImpl* frame_sink_manager_;

  // Properties of the FrameSink. If element is a RootFrameSink then it has
  // |is_root_| = true. If element is not a root than it has |is_root_| = false.
  // If an element is a sibling of a RootFrameSink but has property |is_root_| =
  // false then it is considered detached. If the FrameSink was registered then
  // corresponding element's |is_registered_| = true. If a FrameSink was created
  // then |is_client_connected_| = true.
  bool is_root_;
  bool is_registered_;
  bool is_client_connected_;

  DISALLOW_COPY_AND_ASSIGN(FrameSinkElement);
};

}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_VIZ_VIEWS_FRAME_SINK_ELEMENT_H_
