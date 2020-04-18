// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_HARMONY_BULLETED_LABEL_LIST_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_HARMONY_BULLETED_LABEL_LIST_VIEW_H_

#include "ui/views/view.h"

class BulletedLabelListView : public views::View {
 public:
  BulletedLabelListView();
  explicit BulletedLabelListView(const std::vector<base::string16>& texts);

  ~BulletedLabelListView() override;

  void AddLabel(const base::string16& text);

 private:
  DISALLOW_COPY_AND_ASSIGN(BulletedLabelListView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_HARMONY_BULLETED_LABEL_LIST_VIEW_H_
