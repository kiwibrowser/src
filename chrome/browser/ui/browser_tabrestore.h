// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_TABRESTORE_H_
#define CHROME_BROWSER_UI_BROWSER_TABRESTORE_H_

#include <string>
#include <vector>

#include "components/sessions/core/session_types.h"

class Browser;

namespace content {
class SessionStorageNamespace;
class WebContents;
}

namespace sessions {
class SerializedNavigationEntry;
}

namespace chrome {

// Add a tab with its session history restored from the SessionRestore and
// TabRestoreService systems. If select is true, the tab is selected.
// |tab_index| gives the index to insert the tab at. |selected_navigation| is
// the index of the SerializedNavigationEntry in |navigations| to select. If
// |extension_app_id| is non-empty the tab is an app tab and |extension_app_id|
// is the id of the extension. If |pin| is true and |tab_index|/ is the last
// pinned tab, then the newly created tab is pinned. If |from_last_session| is
// true, |navigations| are from the previous session. |user_agent_override|
// contains the string being used as the user agent for all of the tab's
// navigations when the regular user agent is overridden. If
// |from_session_restore| is true, the restored tab is created by session
// restore. Returns the WebContents of the restored tab.
content::WebContents* AddRestoredTab(
    Browser* browser,
    const std::vector<sessions::SerializedNavigationEntry>& navigations,
    int tab_index,
    int selected_navigation,
    const std::string& extension_app_id,
    bool select,
    bool pin,
    bool from_last_session,
    content::SessionStorageNamespace* storage_namespace,
    const std::string& user_agent_override,
    bool from_session_restore);

// Replaces the state of the currently selected tab with the session
// history restored from the SessionRestore and TabRestoreService systems.
// Returns the WebContents of the restored tab.
content::WebContents* ReplaceRestoredTab(
    Browser* browser,
    const std::vector<sessions::SerializedNavigationEntry>& navigations,
    int selected_navigation,
    bool from_last_session,
    const std::string& extension_app_id,
    content::SessionStorageNamespace* session_storage_namespace,
    const std::string& user_agent_override,
    bool from_session_restore);

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_BROWSER_TABRESTORE_H_
