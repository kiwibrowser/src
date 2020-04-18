// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_CONFLICTING_MODULE_VIEW_WIN_H_
#define CHROME_BROWSER_UI_VIEWS_CONFLICTING_MODULE_VIEW_WIN_H_

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/win/enumerate_modules_model.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "url/gurl.h"

class Browser;

// This is the class that implements the UI for the bubble showing that there
// is a 3rd party module loaded that conflicts with Chrome.
// TODO(pmonette): Delete this view when EnumerateModulesModel gets removed.
class ConflictingModuleView : public views::BubbleDialogDelegateView,
                              public EnumerateModulesModel::Observer {
 public:
  ConflictingModuleView(views::View* anchor_view,
                        Browser* browser,
                        const GURL& help_center_url);

  // Show the Disabled Extension bubble, if needed.
  static void MaybeShow(Browser* browser, views::View* anchor_view);

 private:
  ~ConflictingModuleView() override;

  // Shows the bubble and updates the counter for how often it has been shown.
  void ShowBubble();

  void OnWidgetClosing(views::Widget* widget) override;
  bool Accept() override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  void Init() override;

  // EnumerateModulesModel::Observer:
  void OnConflictsAcknowledged() override;

  Browser* const browser_;

  ScopedObserver<EnumerateModulesModel,
                 EnumerateModulesModel::Observer> observer_;

  // The link to the help center for this conflict.
  GURL help_center_url_;

  DISALLOW_COPY_AND_ASSIGN(ConflictingModuleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_CONFLICTING_MODULE_VIEW_WIN_H_
