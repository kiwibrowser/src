// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_PROPERTY_DATA_H_
#define UI_BASE_PROPERTY_DATA_H_

#include "ui/base/ui_base_export.h"

namespace ui {

// Descendants of ui::PropertyHandler may return a descendant of this class
// when overriding BeforePropertyChange(). This instance is then passed to
// AfterPropertyChange() in order to preserve and/or communicate data between
// those two calls.
struct UI_BASE_EXPORT PropertyData {
  virtual ~PropertyData() {}
};

}

#endif
