// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/content/content_live_tab.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "components/sessions/content/content_platform_specific_tab_data.h"

namespace {
const char kContentLiveTabWebContentsUserDataKey[] = "content_live_tab";
}

namespace sessions {

// static
ContentLiveTab* ContentLiveTab::GetForWebContents(
    content::WebContents* contents) {
  if (!contents->GetUserData(kContentLiveTabWebContentsUserDataKey)) {
    contents->SetUserData(kContentLiveTabWebContentsUserDataKey,
                          base::WrapUnique(new ContentLiveTab(contents)));
  }

  return static_cast<ContentLiveTab*>(contents->GetUserData(
      kContentLiveTabWebContentsUserDataKey));
}

ContentLiveTab::ContentLiveTab(content::WebContents* contents)
    : web_contents_(contents) {}

ContentLiveTab::~ContentLiveTab() {}

bool ContentLiveTab::IsInitialBlankNavigation() {
  return navigation_controller().IsInitialBlankNavigation();
}

int ContentLiveTab::GetCurrentEntryIndex() {
  return navigation_controller().GetCurrentEntryIndex();
}

int ContentLiveTab::GetPendingEntryIndex() {
  return navigation_controller().GetPendingEntryIndex();
}

sessions::SerializedNavigationEntry ContentLiveTab::GetEntryAtIndex(int index) {
  return sessions::ContentSerializedNavigationBuilder::FromNavigationEntry(
      index, *navigation_controller().GetEntryAtIndex(index));
}

sessions::SerializedNavigationEntry ContentLiveTab::GetPendingEntry() {
  return sessions::ContentSerializedNavigationBuilder::FromNavigationEntry(
      GetPendingEntryIndex(), *navigation_controller().GetPendingEntry());
}

int ContentLiveTab::GetEntryCount() {
  return navigation_controller().GetEntryCount();
}

std::unique_ptr<sessions::PlatformSpecificTabData>
ContentLiveTab::GetPlatformSpecificTabData() {
  return std::make_unique<sessions::ContentPlatformSpecificTabData>(
      web_contents());
}

const std::string& ContentLiveTab::GetUserAgentOverride() const {
  return web_contents()->GetUserAgentOverride();
}

}  // namespace sessions
