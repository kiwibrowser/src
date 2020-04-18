// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol/devtools_download_manager_helper.h"

#include "base/bind.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    content::protocol::DevToolsDownloadManagerHelper);

namespace content {
namespace protocol {

DevToolsDownloadManagerHelper::DevToolsDownloadManagerHelper(
    content::WebContents* contents)
    : download_behavior_(
          DevToolsDownloadManagerHelper::DownloadBehavior::DENY) {}

DevToolsDownloadManagerHelper::~DevToolsDownloadManagerHelper() {}

// static
void DevToolsDownloadManagerHelper::RemoveFromWebContents(
    content::WebContents* web_contents) {
  web_contents->RemoveUserData(UserDataKey());
}

DevToolsDownloadManagerHelper::DownloadBehavior
DevToolsDownloadManagerHelper::GetDownloadBehavior() {
  return download_behavior_;
}

void DevToolsDownloadManagerHelper::SetDownloadBehavior(
    DevToolsDownloadManagerHelper::DownloadBehavior behavior) {
  download_behavior_ = behavior;
}

std::string DevToolsDownloadManagerHelper::GetDownloadPath() {
  return download_path_;
}

void DevToolsDownloadManagerHelper::SetDownloadPath(const std::string& path) {
  download_path_ = path;
}

}  // namespace protocol
}  // namespace content
