// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_manager/providers/web_contents/devtools_tag.h"

namespace task_manager {

DevToolsTask* DevToolsTag::CreateTask() const {
  return new DevToolsTask(web_contents());
}

DevToolsTag::DevToolsTag(content::WebContents* web_contents)
    : WebContentsTag(web_contents) {
}

DevToolsTag::~DevToolsTag() {
}

}  // namespace task_manager
