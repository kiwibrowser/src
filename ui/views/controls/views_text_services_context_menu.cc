// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/views_text_services_context_menu.h"

#include <memory>

#include "base/logging.h"
#include "ui/views/controls/views_text_services_context_menu_base.h"

namespace views {

// static
std::unique_ptr<ViewsTextServicesContextMenu>
ViewsTextServicesContextMenu::Create(ui::SimpleMenuModel* menu,
                                     Textfield* client) {
  return std::make_unique<ViewsTextServicesContextMenuBase>(menu, client);
}

bool ViewsTextServicesContextMenu::IsTextDirectionCheckedForTesting(
    ViewsTextServicesContextMenu* menu,
    base::i18n::TextDirection direction) {
  NOTREACHED();
  return false;
}

}  // namespace views