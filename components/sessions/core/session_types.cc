// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/core/session_types.h"

#include <stddef.h>

#include "components/sessions/core/session_command.h"

namespace sessions {

//using class SerializedNavigationEntry;

// SessionTab -----------------------------------------------------------------

SessionTab::SessionTab()
    : window_id(SessionID::NewUnique()),
      tab_id(SessionID::NewUnique()),
      tab_visual_index(-1),
      current_navigation_index(-1),
      pinned(false) {}

SessionTab::~SessionTab() {
}

void SessionTab::SetFromSyncData(const sync_pb::SessionTab& sync_data,
                                 base::Time timestamp) {
  window_id = SessionID::FromSerializedValue(sync_data.window_id());
  tab_id = SessionID::FromSerializedValue(sync_data.tab_id());
  tab_visual_index = sync_data.tab_visual_index();
  current_navigation_index = sync_data.current_navigation_index();
  pinned = sync_data.pinned();
  extension_app_id = sync_data.extension_app_id();
  user_agent_override.clear();
  this->timestamp = timestamp;
  navigations.clear();
  for (int i = 0; i < sync_data.navigation_size(); ++i) {
    navigations.push_back(
        SerializedNavigationEntry::FromSyncData(i, sync_data.navigation(i)));
  }
  session_storage_persistent_id.clear();
}

sync_pb::SessionTab SessionTab::ToSyncData() const {
  sync_pb::SessionTab sync_data;
  sync_data.set_tab_id(tab_id.id());
  sync_data.set_window_id(window_id.id());
  sync_data.set_tab_visual_index(tab_visual_index);
  sync_data.set_current_navigation_index(current_navigation_index);
  sync_data.set_pinned(pinned);
  sync_data.set_extension_app_id(extension_app_id);
  for (const SerializedNavigationEntry& navigation : navigations) {
    *sync_data.add_navigation() = navigation.ToSyncData();
  }
  return sync_data;
}

// SessionWindow ---------------------------------------------------------------

SessionWindow::SessionWindow()
    : window_id(SessionID::NewUnique()),
      selected_tab_index(-1),
      type(TYPE_TABBED),
      is_constrained(true),
      show_state(ui::SHOW_STATE_DEFAULT) {}

SessionWindow::~SessionWindow() {}

}  // namespace sessions
