// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/external_popup_menu.h"

#include <stddef.h>

#include "build/build_config.h"
#include "content/common/frame_messages.h"
#include "content/renderer/menu_item_builder.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/web/web_external_popup_menu_client.h"

namespace content {

ExternalPopupMenu::ExternalPopupMenu(
    RenderFrameImpl* render_frame,
    const blink::WebPopupMenuInfo& popup_menu_info,
    blink::WebExternalPopupMenuClient* popup_menu_client)
    : render_frame_(render_frame),
      popup_menu_info_(popup_menu_info),
      popup_menu_client_(popup_menu_client),
      origin_scale_for_emulation_(0) {
}

void ExternalPopupMenu::SetOriginScaleForEmulation(float scale) {
  origin_scale_for_emulation_ = scale;
}

void ExternalPopupMenu::Show(const blink::WebRect& bounds) {
  blink::WebRect rect = bounds;
  if (origin_scale_for_emulation_) {
    rect.x *= origin_scale_for_emulation_;
    rect.y *= origin_scale_for_emulation_;
  }

  FrameHostMsg_ShowPopup_Params popup_params;
  popup_params.bounds = rect;
  popup_params.item_height = popup_menu_info_.item_height;
  popup_params.item_font_size = popup_menu_info_.item_font_size;
  popup_params.selected_item = popup_menu_info_.selected_index;
  for (size_t i = 0; i < popup_menu_info_.items.size(); ++i) {
    popup_params.popup_items.push_back(
        MenuItemBuilder::Build(popup_menu_info_.items[i]));
  }
  popup_params.right_aligned = popup_menu_info_.right_aligned;
  popup_params.allow_multiple_selection =
      popup_menu_info_.allow_multiple_selection;
  render_frame_->Send(
      new FrameHostMsg_ShowPopup(render_frame_->GetRoutingID(), popup_params));
}

void ExternalPopupMenu::Close() {
  render_frame_->Send(
      new FrameHostMsg_HidePopup(render_frame_->GetRoutingID()));
  render_frame_->DidHideExternalPopupMenu();
  // |this| was deleted.
}

#if BUILDFLAG(USE_EXTERNAL_POPUP_MENU)
#if defined(OS_MACOSX)
void ExternalPopupMenu::DidSelectItem(int index) {
  if (!popup_menu_client_)
    return;
  if (index == -1)
    popup_menu_client_->DidCancel();
  else
    popup_menu_client_->DidAcceptIndex(index);
}
#else
void ExternalPopupMenu::DidSelectItems(bool canceled,
                                       const std::vector<int>& indices) {
  if (!popup_menu_client_)
    return;
  if (canceled)
    popup_menu_client_->DidCancel();
  else
    popup_menu_client_->DidAcceptIndices(indices);
}
#endif
#endif

}  // namespace content
