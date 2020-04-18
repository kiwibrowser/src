// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MD_BOOKMARKS_MD_BOOKMARKS_UI_H_
#define CHROME_BROWSER_UI_WEBUI_MD_BOOKMARKS_MD_BOOKMARKS_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/base/resource/scale_factor.h"

namespace base {
class RefCountedMemory;
}

class MdBookmarksUI : public content::WebUIController {
 public:
  explicit MdBookmarksUI(content::WebUI* web_ui);

  static base::RefCountedMemory* GetFaviconResourceBytes(
      ui::ScaleFactor scale_factor);

 private:
  DISALLOW_COPY_AND_ASSIGN(MdBookmarksUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MD_BOOKMARKS_MD_BOOKMARKS_UI_H_
