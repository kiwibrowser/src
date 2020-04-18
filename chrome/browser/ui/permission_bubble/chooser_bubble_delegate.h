// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PERMISSION_BUBBLE_CHOOSER_BUBBLE_DELEGATE_H_
#define CHROME_BROWSER_UI_PERMISSION_BUBBLE_CHOOSER_BUBBLE_DELEGATE_H_

#include "base/macros.h"
#include "components/bubble/bubble_delegate.h"

class Browser;
class ChooserController;

namespace content {
class RenderFrameHost;
}

// ChooserBubbleDelegate overrides GetName() to identify the bubble
// you define for collecting metrics. Create an instance of this
// class and pass it to BubbleManager::ShowBubble() to show the bubble.
class ChooserBubbleDelegate : public BubbleDelegate {
 public:
  ChooserBubbleDelegate(content::RenderFrameHost* owner,
                        std::unique_ptr<ChooserController> chooser_controller);
  ~ChooserBubbleDelegate() override;

  // BubbleDelegate:
  std::string GetName() const override;
  std::unique_ptr<BubbleUi> BuildBubbleUi() override;
  const content::RenderFrameHost* OwningFrame() const override;

 private:
  const content::RenderFrameHost* const owning_frame_;
  Browser* const browser_;
  // |chooser_controller_| is not owned by this class, it is owned by
  // DeviceChooserContentView[Cocoa].
  // This field only temporarily owns the ChooserController. It is moved
  // into the DeviceChooserContentView[Cocoa] when BuildBubbleUi() is called
  // and the bubble is shown.
  std::unique_ptr<ChooserController> chooser_controller_;

  DISALLOW_COPY_AND_ASSIGN(ChooserBubbleDelegate);
};

#endif  // CHROME_BROWSER_UI_PERMISSION_BUBBLE_CHOOSER_BUBBLE_DELEGATE_H_
