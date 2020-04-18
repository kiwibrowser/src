// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_SEARCH_RESULT_BASE_VIEW_H_
#define UI_APP_LIST_VIEWS_SEARCH_RESULT_BASE_VIEW_H_

#include "ash/app_list/model/search/search_result_observer.h"
#include "ui/app_list/app_list_export.h"
#include "ui/views/controls/button/button.h"

namespace app_list {

class APP_LIST_EXPORT SearchResultBaseView : public views::Button,
                                             public views::ButtonListener,
                                             public SearchResultObserver {
 public:
  SearchResultBaseView();

  // Set or remove the background highlight.
  void SetBackgroundHighlighted(bool enabled);

  bool background_highlighted() const { return background_highlighted_; }

 protected:
  ~SearchResultBaseView() override;

 private:
  bool background_highlighted_ = false;

  DISALLOW_COPY_AND_ASSIGN(SearchResultBaseView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_SEARCH_RESULT_BASE_VIEW_H_
