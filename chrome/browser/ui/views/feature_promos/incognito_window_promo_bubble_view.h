// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_INCOGNITO_WINDOW_PROMO_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_INCOGNITO_WINDOW_PROMO_BUBBLE_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/feature_promos/feature_promo_bubble_view.h"

// The IncognitoWindowPromoBubbleView is a bubble anchored to the right of the
// App Menu Button. It draws users' attention to the App Menu Button. It is
// created by the App Menu Button when prompted by the IncognitoWindowTracker.
class IncognitoWindowPromoBubbleView : public FeaturePromoBubbleView {
 public:
  // Returns a raw pointer that is owned by its native widget.
  static IncognitoWindowPromoBubbleView* CreateOwned(views::View* anchor_view);

 private:
  // Anchors the bubble to |anchor_view|. The bubble widget and promo are
  // owned by their native widget.
  explicit IncognitoWindowPromoBubbleView(views::View* anchor_view);
  ~IncognitoWindowPromoBubbleView() override;

  // Returns the string ID to display in the promo.
  int GetStringSpecifier();

  DISALLOW_COPY_AND_ASSIGN(IncognitoWindowPromoBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_INCOGNITO_WINDOW_PROMO_BUBBLE_VIEW_H_
