// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_TOOLBAR_MODEL_DELEGATE_H_
#define CHROME_BROWSER_UI_BROWSER_TOOLBAR_MODEL_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/ui/toolbar/chrome_toolbar_model_delegate.h"

class Browser;

// Implementation of ToolbarModelDelegate which uses an instance of
// Browser in order to fulfil its duties.
class BrowserToolbarModelDelegate : public ChromeToolbarModelDelegate {
 public:
  explicit BrowserToolbarModelDelegate(Browser* browser);
  ~BrowserToolbarModelDelegate() override;

  // ChromeToolbarModelDelegate:
  content::WebContents* GetActiveWebContents() const override;

 private:
  Browser* const browser_;

  DISALLOW_COPY_AND_ASSIGN(BrowserToolbarModelDelegate);
};

#endif  // CHROME_BROWSER_UI_BROWSER_TOOLBAR_MODEL_DELEGATE_H_
