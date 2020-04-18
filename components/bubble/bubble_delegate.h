// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BUBBLE_BUBBLE_DELEGATE_H_
#define COMPONENTS_BUBBLE_BUBBLE_DELEGATE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/bubble/bubble_close_reason.h"

class BubbleUi;

namespace content {
class RenderFrameHost;
}

// Inherit from this class to define a bubble. A bubble is a small transient UI
// surface anchored to a parent window. Most bubbles are dismissed when they
// lose focus.
class BubbleDelegate {
 public:
  BubbleDelegate();
  virtual ~BubbleDelegate();

  // Called by BubbleController to notify a bubble of an event that the bubble
  // might want to close on. Return true if the bubble should close for the
  // specified reason.
  virtual bool ShouldClose(BubbleCloseReason reason) const;

  // Called by BubbleController to notify a bubble that it has closed.
  virtual void DidClose(BubbleCloseReason reason);

  // Called by BubbleController to build the UI that will represent this bubble.
  // BubbleDelegate should not keep a reference to this newly created UI.
  virtual std::unique_ptr<BubbleUi> BuildBubbleUi() = 0;

  // Called to update an existing UI. This is the same BubbleUi that was created
  // in |BuildBubbleUi|.
  // Return true to indicate the UI was updated.
  virtual bool UpdateBubbleUi(BubbleUi* bubble_ui);

  // Used to identify a bubble for collecting metrics.
  virtual std::string GetName() const = 0;

  // If this returns non-null, the bubble will be closed when the returned frame
  // is destroyed.
  virtual const content::RenderFrameHost* OwningFrame() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(BubbleDelegate);
};

#endif  // COMPONENTS_BUBBLE_BUBBLE_DELEGATE_H_
