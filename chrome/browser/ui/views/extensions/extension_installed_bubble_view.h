// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_INSTALLED_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_INSTALLED_BUBBLE_VIEW_H_

#include "base/macros.h"
#include "components/bubble/bubble_ui.h"
#include "ui/views/widget/widget_observer.h"

class ExtensionInstalledBubble;
class ExtensionInstalledBubbleView;

// NB: This bubble is using the temporarily-deprecated bubble manager interface
// BubbleUi. Do not copy this pattern.
class ExtensionInstalledBubbleUi : public BubbleUi,
                                   public views::WidgetObserver {
 public:
  explicit ExtensionInstalledBubbleUi(ExtensionInstalledBubble* bubble);
  ~ExtensionInstalledBubbleUi() override;

  ExtensionInstalledBubbleView* bubble_view() { return bubble_view_; }

  // BubbleUi:
  void Show(BubbleReference bubble_reference) override;
  void Close() override;
  void UpdateAnchorPosition() override;

  // WidgetObserver:
  void OnWidgetClosing(views::Widget* widget) override;

 private:
  ExtensionInstalledBubble* bubble_;
  ExtensionInstalledBubbleView* bubble_view_;
  BubbleReference bubble_reference_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionInstalledBubbleUi);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_INSTALLED_BUBBLE_VIEW_H_
