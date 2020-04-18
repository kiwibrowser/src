// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_PRINTING_SETTINGS_INITIALIZER_WIN_H_
#define PRINTING_PRINTING_SETTINGS_INITIALIZER_WIN_H_

#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "printing/page_range.h"

typedef struct HDC__* HDC;
typedef struct _devicemodeW DEVMODE;

namespace printing {

class PrintSettings;

// Initializes a PrintSettings object from the provided device context.
class PRINTING_EXPORT PrintSettingsInitializerWin {
 public:
  static void InitPrintSettings(HDC hdc,
                                const DEVMODE& dev_mode,
                                PrintSettings* print_settings);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PrintSettingsInitializerWin);
};

}  // namespace printing

#endif  // PRINTING_PRINTING_SETTINGS_INITIALIZER_WIN_H_
