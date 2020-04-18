// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/views_text_services_context_menu_base.h"

#include "ui/base/emoji/emoji_panel_helper.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/controls/textfield/textfield.h"

namespace views {

ViewsTextServicesContextMenuBase::ViewsTextServicesContextMenuBase(
    ui::SimpleMenuModel* menu,
    Textfield* client)
    : client_(client) {
  DCHECK(client);
  DCHECK(menu);
  // Not inserted on read-only fields or if the OS/version doesn't support it.
  if (!client_->read_only() && ui::IsEmojiPanelSupported()) {
    menu->InsertSeparatorAt(0, ui::NORMAL_SEPARATOR);
    menu->InsertItemWithStringIdAt(0, IDS_CONTENT_CONTEXT_EMOJI,
                                   IDS_CONTENT_CONTEXT_EMOJI);
  }
}

ViewsTextServicesContextMenuBase::~ViewsTextServicesContextMenuBase() {}

bool ViewsTextServicesContextMenuBase::SupportsCommand(int command_id) const {
  return command_id == IDS_CONTENT_CONTEXT_EMOJI;
}

bool ViewsTextServicesContextMenuBase::IsCommandIdChecked(
    int command_id) const {
  return false;
}

bool ViewsTextServicesContextMenuBase::IsCommandIdEnabled(
    int command_id) const {
  if (command_id == IDS_CONTENT_CONTEXT_EMOJI)
    return true;

  return false;
}

void ViewsTextServicesContextMenuBase::ExecuteCommand(int command_id) {
  if (command_id == IDS_CONTENT_CONTEXT_EMOJI)
    ui::ShowEmojiPanel();
}

}  // namespace views