// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_STATE_LIST_WEB_STATE_LIST_ORDER_CONTROLLER_H_
#define IOS_CHROME_BROWSER_WEB_STATE_LIST_WEB_STATE_LIST_ORDER_CONTROLLER_H_

#include "base/macros.h"

class WebStateList;

namespace web {
class WebState;
}

// WebStateListOrderController allows different types of ordering and
// selection heuristics to be plugged into WebStateList.
class WebStateListOrderController {
 public:
  explicit WebStateListOrderController(WebStateList* web_state_list);
  ~WebStateListOrderController();

  // Determines where to place a newly opened WebState given its opener.
  int DetermineInsertionIndex(web::WebState* opener) const;

  // Determines where to shift the active index after a WebState is closed.
  int DetermineNewActiveIndex(int removing_index) const;

 private:
  // Returns a valid index to be selected after the WebState at |removing_index|
  // is detached, adjusting |index| to reflect that |removing_index| is going
  // away.
  int GetValidIndex(int index, int removing_index) const;

  WebStateList* web_state_list_;

  DISALLOW_COPY_AND_ASSIGN(WebStateListOrderController);
};

#endif  // IOS_CHROME_BROWSER_WEB_STATE_LIST_WEB_STATE_LIST_ORDER_CONTROLLER_H_
