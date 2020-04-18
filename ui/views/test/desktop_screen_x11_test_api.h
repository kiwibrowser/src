// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_TEST_DESKTOP_SCREEN_X11_TEST_API_H_
#define UI_VIEWS_TEST_DESKTOP_SCREEN_X11_TEST_API_H_

#include "base/macros.h"

namespace views {
namespace test {

class DesktopScreenX11TestApi {
 public:
  // Updates the displays. Typically used to reflect customized device
  // scale factor in displays.
  static void UpdateDisplays();

 private:
  DISALLOW_COPY_AND_ASSIGN(DesktopScreenX11TestApi);
};

}  // namespace test
}  // namespace views

#endif  // UI_VIEWS_TEST_DESKTOP_SCREEN_X11_TEST_API_H_
