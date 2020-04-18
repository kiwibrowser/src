// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "build/buildflag.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/bubble_anchor_helper.h"
#include "chrome/browser/ui/cocoa/bubble_anchor_helper_views.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#include "chrome/browser/ui/views/permission_bubble/permission_prompt_impl.h"
#include "chrome/browser/ui/views_mode_controller.h"
#include "ui/base/ui_features.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace {

views::BubbleDialogDelegateView* BubbleForWindow(gfx::NativeWindow window) {
  views::Widget* widget = views::Widget::GetWidgetForNativeWindow(window);
  DCHECK(widget);
  return widget->widget_delegate()->AsBubbleDialogDelegate();
}

}  // namespace

// static
std::unique_ptr<PermissionPrompt> PermissionPrompt::Create(
    content::WebContents* web_contents,
    Delegate* delegate) {
#if BUILDFLAG(MAC_VIEWS_BROWSER)
  if (!views_mode_controller::IsViewsBrowserCocoa()) {
    return base::WrapUnique(new PermissionPromptImpl(
        chrome::FindBrowserWithWebContents(web_contents), delegate));
  }
#endif
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  auto prompt = std::make_unique<PermissionPromptImpl>(browser, delegate);
  // Note the PermissionPromptImpl constructor always shows the bubble, which
  // is necessary to call TrackBubbleState().
  // Also note it's important to use BrowserWindow::GetNativeWindow() and not
  // WebContents::GetTopLevelNativeWindow() below, since there's a brief
  // period when attaching a dragged tab to a window that WebContents thinks
  // it hasn't yet moved to the new window.
  TrackBubbleState(BubbleForWindow(prompt->GetNativeWindow()),
                   GetPageInfoDecoration(browser->window()->GetNativeWindow()));
  return prompt;
}
