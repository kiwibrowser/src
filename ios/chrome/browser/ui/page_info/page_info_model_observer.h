// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_MODEL_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_MODEL_OBSERVER_H_

// TODO(crbug.com/227827) Merge 178763: PageInfoModel has been removed in
// upstream; check if we should use PageInfoModel.
// This interface should be implemented by classes interested in getting
// notifications from PageInfoModel.
class PageInfoModelObserver {
 public:
  virtual ~PageInfoModelObserver() {}

  virtual void OnPageInfoModelChanged() = 0;
};

#endif  // IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_MODEL_OBSERVER_H_
