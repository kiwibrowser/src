// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_manager/providers/web_contents/tab_contents_tag.h"

namespace task_manager {

TabContentsTask* TabContentsTag::CreateTask() const {
  return new TabContentsTask(web_contents());
}

TabContentsTag::TabContentsTag(content::WebContents* web_contents)
    : WebContentsTag(web_contents) {
}

TabContentsTag::~TabContentsTag() {
}

}  // namespace task_manager
