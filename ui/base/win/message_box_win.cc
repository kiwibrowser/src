// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/message_box_win.h"

#include "base/i18n/rtl.h"

namespace ui {

// In addition to passing the RTL flags to ::MessageBox if we are running in an
// RTL locale, we need to make sure that LTR strings are rendered correctly by
// adding the appropriate Unicode directionality marks.
int MessageBox(HWND hwnd,
               const base::string16& text,
               const base::string16& caption,
               UINT flags) {
  UINT actual_flags = flags;
  if (base::i18n::IsRTL())
    actual_flags |= MB_RIGHT | MB_RTLREADING;

  base::string16 localized_text = text;
  base::i18n::AdjustStringForLocaleDirection(&localized_text);
  const wchar_t* text_ptr = localized_text.c_str();

  base::string16 localized_caption = caption;
  base::i18n::AdjustStringForLocaleDirection(&localized_caption);
  const wchar_t* caption_ptr = localized_caption.c_str();

  return ::MessageBox(hwnd, text_ptr, caption_ptr, actual_flags);
}

}  // namespace ui
