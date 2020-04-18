// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/permission_bubble/chooser_bubble_delegate.h"

#include "chrome/browser/chooser_controller/chooser_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "content/public/browser/web_contents.h"

ChooserBubbleDelegate::ChooserBubbleDelegate(
    content::RenderFrameHost* owner,
    std::unique_ptr<ChooserController> chooser_controller)
    : owning_frame_(owner),
      browser_(chrome::FindBrowserWithWebContents(
          content::WebContents::FromRenderFrameHost(owner))),
      chooser_controller_(std::move(chooser_controller)) {}

ChooserBubbleDelegate::~ChooserBubbleDelegate() {}

std::string ChooserBubbleDelegate::GetName() const {
  return "ChooserBubble";
}

const content::RenderFrameHost* ChooserBubbleDelegate::OwningFrame() const {
  return owning_frame_;
}
