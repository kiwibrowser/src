// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/chrome_extension_api_frame_id_map_helper.h"

#include "base/bind.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/constants.h"

namespace extensions {

ChromeExtensionApiFrameIdMapHelper::ChromeExtensionApiFrameIdMapHelper(
    ExtensionApiFrameIdMap* owner)
    : owner_(owner) {
  registrar_.Add(this, chrome::NOTIFICATION_TAB_PARENTED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

ChromeExtensionApiFrameIdMapHelper::~ChromeExtensionApiFrameIdMapHelper() =
    default;

// static
bool ChromeExtensionApiFrameIdMapHelper::PopulateTabData(
    content::WebContents* web_contents,
    int* tab_id,
    int* window_id) {
  if (!web_contents)
    return false;

  SessionTabHelper* session_tab_helper =
      SessionTabHelper::FromWebContents(web_contents);
  if (!session_tab_helper)
    return false;

  *tab_id = session_tab_helper->session_id().id();
  *window_id = session_tab_helper->window_id().id();
  return true;
}

void ChromeExtensionApiFrameIdMapHelper::PopulateTabData(
    content::RenderFrameHost* rfh,
    int* tab_id,
    int* window_id) {
  PopulateTabData(content::WebContents::FromRenderFrameHost(rfh), tab_id,
                  window_id);
}

void ChromeExtensionApiFrameIdMapHelper::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_TAB_PARENTED, type);
  content::WebContents* web_contents =
      content::Source<content::WebContents>(source).ptr();

  int tab_id = extension_misc::kUnknownTabId;
  int window_id = extension_misc::kUnknownWindowId;
  if (!PopulateTabData(web_contents, &tab_id, &window_id))
    return;

  web_contents->ForEachFrame(
      base::BindRepeating(&ExtensionApiFrameIdMap::UpdateTabAndWindowId,
                          base::Unretained(owner_), tab_id, window_id));
}

}  // namespace extensions
