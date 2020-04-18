// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_manager/providers/web_contents/guest_tag.h"

namespace task_manager {

GuestTask* GuestTag::CreateTask() const {
  return new GuestTask(web_contents());
}

GuestTag::GuestTag(content::WebContents* web_contents)
    : WebContentsTag(web_contents) {
}

GuestTag::~GuestTag() {
}

}  // namespace task_manager
