// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_context_menu/context_menu_content_type_panel.h"

ContextMenuContentTypePanel::ContextMenuContentTypePanel(
    content::WebContents* web_contents,
    const content::ContextMenuParams& params)
    : ContextMenuContentType(web_contents, params, false) {
}

ContextMenuContentTypePanel::~ContextMenuContentTypePanel() {
}

bool ContextMenuContentTypePanel::SupportsGroup(int group) {
  switch (group) {
    case ITEM_GROUP_LINK:
      // Checking link should take precedence before checking selection since on
      // Mac right-clicking a link will also make it selected.
      return params().unfiltered_link_url.is_valid();
    case ITEM_GROUP_EDITABLE:
    case ITEM_GROUP_COPY:
    case ITEM_GROUP_SEARCH_PROVIDER:
      return ContextMenuContentType::SupportsGroup(group);
    case ITEM_GROUP_CURRENT_EXTENSION:
      return true;
    default:
      return false;
  }
}
