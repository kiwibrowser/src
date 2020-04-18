// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Stores per-profile state needed for find in page.  This includes the most
// recently searched for term.

#ifndef CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_STATE_H_
#define CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_STATE_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/keyed_service/core/keyed_service.h"

class FindBarState : public KeyedService {
 public:
  FindBarState() {}
  ~FindBarState() override {}

  base::string16 last_prepopulate_text() const {
    return last_prepopulate_text_;
  }

  void set_last_prepopulate_text(const base::string16& text) {
    last_prepopulate_text_ = text;
  }

 private:
  base::string16 last_prepopulate_text_;

  DISALLOW_COPY_AND_ASSIGN(FindBarState);
};

#endif  // CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_STATE_H_
