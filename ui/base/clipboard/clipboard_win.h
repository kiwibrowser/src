// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_CLIPBOARD_CLIPBOARD_WIN_H_
#define UI_BASE_CLIPBOARD_CLIPBOARD_WIN_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "ui/base/clipboard/clipboard.h"

namespace base {
namespace win {
class MessageWindow;
}
}

namespace gfx {
class Size;
}

namespace ui {

class ClipboardWin : public Clipboard {
 private:
  friend class Clipboard;

  ClipboardWin();
  ~ClipboardWin() override;

  // Clipboard overrides:
  void OnPreShutdown() override;
  uint64_t GetSequenceNumber(ClipboardType type) const override;
  bool IsFormatAvailable(const FormatType& format,
                         ClipboardType type) const override;
  void Clear(ClipboardType type) override;
  void ReadAvailableTypes(ClipboardType type,
                          std::vector<base::string16>* types,
                          bool* contains_filenames) const override;
  void ReadText(ClipboardType type, base::string16* result) const override;
  void ReadAsciiText(ClipboardType type, std::string* result) const override;
  void ReadHTML(ClipboardType type,
                base::string16* markup,
                std::string* src_url,
                uint32_t* fragment_start,
                uint32_t* fragment_end) const override;
  void ReadRTF(ClipboardType type, std::string* result) const override;
  SkBitmap ReadImage(ClipboardType type) const override;
  void ReadCustomData(ClipboardType clipboard_type,
                      const base::string16& type,
                      base::string16* result) const override;
  void ReadBookmark(base::string16* title, std::string* url) const override;
  void ReadData(const FormatType& format, std::string* result) const override;
  void WriteObjects(ClipboardType type, const ObjectMap& objects) override;
  void WriteText(const char* text_data, size_t text_len) override;
  void WriteHTML(const char* markup_data,
                 size_t markup_len,
                 const char* url_data,
                 size_t url_len) override;
  void WriteRTF(const char* rtf_data, size_t data_len) override;
  void WriteBookmark(const char* title_data,
                     size_t title_len,
                     const char* url_data,
                     size_t url_len) override;
  void WriteWebSmartPaste() override;
  void WriteBitmap(const SkBitmap& bitmap) override;
  void WriteData(const FormatType& format,
                 const char* data_data,
                 size_t data_len) override;

  void WriteBitmapFromHandle(HBITMAP source_hbitmap, const gfx::Size& size);

  // Safely write to system clipboard. Free |handle| on failure.
  void WriteToClipboard(unsigned int format, HANDLE handle);

  // Return the window that should be the clipboard owner, creating it
  // if neccessary.  Marked const for lazily initialization by const methods.
  HWND GetClipboardWindow() const;

  // Mark this as mutable so const methods can still do lazy initialization.
  mutable std::unique_ptr<base::win::MessageWindow> clipboard_owner_;

  DISALLOW_COPY_AND_ASSIGN(ClipboardWin);
};

}  // namespace ui

#endif  // UI_BASE_CLIPBOARD_CLIPBOARD_WIN_H_
