// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ui_devtools/viz_views/dom_agent_viz.h"

#include "components/ui_devtools/root_element.h"
#include "components/ui_devtools/ui_element.h"
#include "components/ui_devtools/viz_views/frame_sink_element.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"

namespace ui_devtools {

using namespace ui_devtools::protocol;

DOMAgentViz::DOMAgentViz(viz::FrameSinkManagerImpl* frame_sink_manager)
    : frame_sink_manager_(frame_sink_manager) {}

DOMAgentViz::~DOMAgentViz() {
  Clear();
}

std::unique_ptr<DOM::Node> DOMAgentViz::BuildTreeForFrameSink(
    UIElement* frame_sink_element,
    const viz::FrameSinkId& frame_sink_id) {
  frame_sink_elements_.emplace(frame_sink_id, frame_sink_element);
  std::unique_ptr<Array<DOM::Node>> children = Array<DOM::Node>::create();

  // Once the FrameSinkElement is created it calls this function to build its
  // subtree. So we iterate through |frame_sink_element|'s children and
  // recursively build the subtree for them.
  for (auto& child : frame_sink_manager_->GetChildrenByParent(frame_sink_id)) {
    bool is_registered = registered_frame_sink_ids_.find(child) !=
                         registered_frame_sink_ids_.end();
    bool is_client_connected = client_connected_frame_sinks_.find(child) !=
                               client_connected_frame_sinks_.end();

    UIElement* f_s_element = new FrameSinkElement(
        child, frame_sink_manager_, this, frame_sink_element, false /*is_root*/,
        is_registered, is_client_connected);

    children->addItem(BuildTreeForFrameSink(f_s_element, child));
    frame_sink_element->AddChild(f_s_element);
  }
  std::unique_ptr<DOM::Node> node =
      BuildNode("FrameSink", frame_sink_element->GetAttributes(),
                std::move(children), frame_sink_element->node_id());
  return node;
}

protocol::Response DOMAgentViz::enable() {
  InitFrameSinkSets();
  return protocol::Response::OK();
}

protocol::Response DOMAgentViz::disable() {
  Clear();
  return DOMAgent::disable();
}

std::vector<UIElement*> DOMAgentViz::CreateChildrenForRoot() {
  std::vector<UIElement*> children;

  // Step 1. Add created RootFrameSinks and detached FrameSinks.
  for (auto& frame_sink_id : client_connected_frame_sinks_) {
    const viz::CompositorFrameSinkSupport* support =
        frame_sink_manager_->GetFrameSinkForId(frame_sink_id);
    // Do nothing if it's a non-detached non-root FrameSink.
    if (support && !support->is_root() &&
        attached_frame_sinks_.find(frame_sink_id) !=
            attached_frame_sinks_.end())
      continue;

    bool is_registered = registered_frame_sink_ids_.find(frame_sink_id) !=
                         registered_frame_sink_ids_.end();
    bool is_root = support && support->is_root();

    UIElement* frame_sink_element = new FrameSinkElement(
        frame_sink_id, frame_sink_manager_, this, element_root(), is_root,
        is_registered, true /*is_client_connected*/);
    children.push_back(frame_sink_element);
  }

  // Step 2. Add registered but not created FrameSinks. If a FrameSinkId was
  // registered but not created we don't really know whether it's a root or not.
  // And we don't know any information about the hierarchy. Therefore we process
  // FrameSinks that are in the correct state first and only after that we
  // process registered but not created FrameSinks. We consider them unembedded
  // as well.
  for (auto& frame_sink_id : registered_frame_sink_ids_) {
    if (client_connected_frame_sinks_.find(frame_sink_id) !=
        client_connected_frame_sinks_.end())
      continue;

    UIElement* frame_sink_element = new FrameSinkElement(
        frame_sink_id, frame_sink_manager_, this, element_root(),
        false /*is_root*/, true /*is_registered*/,
        false /*is_client_connected*/);

    children.push_back(frame_sink_element);
  }

  return children;
}

std::unique_ptr<DOM::Node> DOMAgentViz::BuildTreeForUIElement(
    UIElement* ui_element) {
  if (ui_element->type() == UIElementType::FRAMESINK) {
    return BuildTreeForFrameSink(ui_element,
                                 FrameSinkElement::From(ui_element));
  }
  return nullptr;
}

void DOMAgentViz::Clear() {
  attached_frame_sinks_.clear();
  client_connected_frame_sinks_.clear();
  frame_sink_elements_.clear();
  registered_frame_sink_ids_.clear();
}

void DOMAgentViz::InitFrameSinkSets() {
  for (auto& entry : frame_sink_manager_->GetRegisteredFrameSinkIds())
    registered_frame_sink_ids_.insert(entry);
  for (auto& entry : frame_sink_manager_->GetCreatedFrameSinkIds())
    client_connected_frame_sinks_.insert(entry);

  // Init the |attached_frame_sinks_| set. All RootFrameSinks and accessible
  // from roots are attached. All the others are detached.
  for (auto& frame_sink_id : client_connected_frame_sinks_) {
    const viz::CompositorFrameSinkSupport* support =
        frame_sink_manager_->GetFrameSinkForId(frame_sink_id);
    // Start only from roots.
    if (!support || !support->is_root())
      continue;

    SetAttachedFrameSink(frame_sink_id);
  }
}

void DOMAgentViz::SetAttachedFrameSink(const viz::FrameSinkId& frame_sink_id) {
  attached_frame_sinks_.insert(frame_sink_id);
  for (auto& child : frame_sink_manager_->GetChildrenByParent(frame_sink_id))
    SetAttachedFrameSink(child);
}

}  // namespace ui_devtools
