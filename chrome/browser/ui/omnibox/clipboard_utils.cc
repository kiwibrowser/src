// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/omnibox/clipboard_utils.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "ui/base/clipboard/clipboard.h"

base::string16 GetClipboardText() {
  // Try text format.
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  if (clipboard->IsFormatAvailable(ui::Clipboard::GetPlainTextWFormatType(),
                                   ui::CLIPBOARD_TYPE_COPY_PASTE)) {
    base::string16 text;
    clipboard->ReadText(ui::CLIPBOARD_TYPE_COPY_PASTE, &text);
    return OmniboxView::SanitizeTextForPaste(text);
  }

  // Try bookmark format.
  //
  // It is tempting to try bookmark format first, but the URL we get out of a
  // bookmark has been cannonicalized via GURL.  This means if a user copies
  // and pastes from the URL bar to itself, the text will get fixed up and
  // cannonicalized, which is not what the user expects.  By pasting in this
  // order, we are sure to paste what the user copied.
  if (clipboard->IsFormatAvailable(ui::Clipboard::GetUrlWFormatType(),
                                   ui::CLIPBOARD_TYPE_COPY_PASTE)) {
    std::string url_str;
    clipboard->ReadBookmark(NULL, &url_str);
    // pass resulting url string through GURL to normalize
    GURL url(url_str);
    if (url.is_valid())
      return OmniboxView::StripJavascriptSchemas(base::UTF8ToUTF16(url.spec()));
  }

  return base::string16();
}
