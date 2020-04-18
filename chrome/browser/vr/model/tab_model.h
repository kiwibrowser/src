// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_TAB_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_TAB_MODEL_H_

#include "base/macros.h"
#include "base/strings/string16.h"

namespace vr {

struct TabModel {
  TabModel(int id, const base::string16& title);
  TabModel(const TabModel& other);
  ~TabModel();

  int id;
  base::string16 title;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_TAB_MODEL_H_
