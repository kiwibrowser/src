// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/page_navigator.h"

namespace content {

OpenURLParams::OpenURLParams(const GURL& url,
                             const Referrer& referrer,
                             WindowOpenDisposition disposition,
                             ui::PageTransition transition,
                             bool is_renderer_initiated)
    : url(url),
      referrer(referrer),
      uses_post(false),
      frame_tree_node_id(RenderFrameHost::kNoFrameTreeNodeId),
      disposition(disposition),
      transition(transition),
      is_renderer_initiated(is_renderer_initiated),
      should_replace_current_entry(false),
      user_gesture(!is_renderer_initiated),
      started_from_context_menu(false),
      open_app_window_if_possible(false) {}

OpenURLParams::OpenURLParams(const GURL& url,
                             const Referrer& referrer,
                             WindowOpenDisposition disposition,
                             ui::PageTransition transition,
                             bool is_renderer_initiated,
                             bool started_from_context_menu)
    : url(url),
      referrer(referrer),
      uses_post(false),
      frame_tree_node_id(RenderFrameHost::kNoFrameTreeNodeId),
      disposition(disposition),
      transition(transition),
      is_renderer_initiated(is_renderer_initiated),
      should_replace_current_entry(false),
      user_gesture(!is_renderer_initiated),
      started_from_context_menu(started_from_context_menu),
      open_app_window_if_possible(false) {}

OpenURLParams::OpenURLParams(const GURL& url,
                             const Referrer& referrer,
                             int frame_tree_node_id,
                             WindowOpenDisposition disposition,
                             ui::PageTransition transition,
                             bool is_renderer_initiated)
    : url(url),
      referrer(referrer),
      uses_post(false),
      frame_tree_node_id(frame_tree_node_id),
      disposition(disposition),
      transition(transition),
      is_renderer_initiated(is_renderer_initiated),
      should_replace_current_entry(false),
      user_gesture(!is_renderer_initiated),
      started_from_context_menu(false),
      open_app_window_if_possible(false) {}

OpenURLParams::OpenURLParams()
    : uses_post(false),
      frame_tree_node_id(RenderFrameHost::kNoFrameTreeNodeId),
      disposition(WindowOpenDisposition::UNKNOWN),
      transition(ui::PAGE_TRANSITION_LINK),
      is_renderer_initiated(false),
      should_replace_current_entry(false),
      user_gesture(true),
      started_from_context_menu(false),
      open_app_window_if_possible(false) {}

OpenURLParams::OpenURLParams(const OpenURLParams& other) = default;

OpenURLParams::~OpenURLParams() {
}

}  // namespace content
