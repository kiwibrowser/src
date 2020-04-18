// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file implements the ScopedClipboardWriter class. Documentation on its
// purpose can be found in our header. Documentation on the format of the
// parameters for each clipboard target can be found in clipboard.h.

#include "ui/base/clipboard/scoped_clipboard_writer.h"

#include "base/pickle.h"
#include "base/strings/utf_string_conversions.h"
#include "net/base/escape.h"
#include "ui/gfx/geometry/size.h"

namespace ui {

ScopedClipboardWriter::ScopedClipboardWriter(ClipboardType type) : type_(type) {
}

ScopedClipboardWriter::~ScopedClipboardWriter() {
  if (!objects_.empty())
    ui::Clipboard::GetForCurrentThread()->WriteObjects(type_, objects_);
}

void ScopedClipboardWriter::WriteText(const base::string16& text) {
  std::string utf8_text = base::UTF16ToUTF8(text);

  Clipboard::ObjectMapParams parameters;
  parameters.push_back(
      Clipboard::ObjectMapParam(utf8_text.begin(), utf8_text.end()));
  objects_[Clipboard::CBF_TEXT] = parameters;
}

void ScopedClipboardWriter::WriteHTML(const base::string16& markup,
                                      const std::string& source_url) {
  std::string utf8_markup = base::UTF16ToUTF8(markup);

  Clipboard::ObjectMapParams parameters;
  parameters.push_back(
      Clipboard::ObjectMapParam(utf8_markup.begin(),
                                utf8_markup.end()));
  if (!source_url.empty()) {
    parameters.push_back(Clipboard::ObjectMapParam(source_url.begin(),
                                                   source_url.end()));
  }

  objects_[Clipboard::CBF_HTML] = parameters;
}

void ScopedClipboardWriter::WriteRTF(const std::string& rtf_data) {
  Clipboard::ObjectMapParams parameters;
  parameters.push_back(Clipboard::ObjectMapParam(rtf_data.begin(),
                                                 rtf_data.end()));
  objects_[Clipboard::CBF_RTF] = parameters;
}

void ScopedClipboardWriter::WriteBookmark(const base::string16& bookmark_title,
                                          const std::string& url) {
  if (bookmark_title.empty() || url.empty())
    return;

  std::string utf8_markup = base::UTF16ToUTF8(bookmark_title);

  Clipboard::ObjectMapParams parameters;
  parameters.push_back(Clipboard::ObjectMapParam(utf8_markup.begin(),
                                                 utf8_markup.end()));
  parameters.push_back(Clipboard::ObjectMapParam(url.begin(), url.end()));
  objects_[Clipboard::CBF_BOOKMARK] = parameters;
}

void ScopedClipboardWriter::WriteHyperlink(const base::string16& anchor_text,
                                           const std::string& url) {
  if (anchor_text.empty() || url.empty())
    return;

  // Construct the hyperlink.
  std::string html("<a href=\"");
  html.append(net::EscapeForHTML(url));
  html.append("\">");
  html.append(net::EscapeForHTML(base::UTF16ToUTF8(anchor_text)));
  html.append("</a>");
  WriteHTML(base::UTF8ToUTF16(html), std::string());
}

void ScopedClipboardWriter::WriteWebSmartPaste() {
  objects_[Clipboard::CBF_WEBKIT] = Clipboard::ObjectMapParams();
}

void ScopedClipboardWriter::WriteImage(const SkBitmap& bitmap) {
  if (bitmap.drawsNothing()) {
    return;
  }
  bitmap_ = bitmap;
  // TODO(dcheng): This is slightly less horrible than what we used to do, but
  // only very slightly less.
  SkBitmap* bitmap_pointer = &bitmap_;
  Clipboard::ObjectMapParam packed_pointer;
  packed_pointer.resize(sizeof(bitmap_pointer));
  *reinterpret_cast<SkBitmap**>(&*packed_pointer.begin()) = bitmap_pointer;
  Clipboard::ObjectMapParams parameters;
  parameters.push_back(packed_pointer);
  objects_[Clipboard::CBF_SMBITMAP] = parameters;
}

void ScopedClipboardWriter::WritePickledData(
    const base::Pickle& pickle,
    const Clipboard::FormatType& format) {
  std::string format_string = format.Serialize();
  Clipboard::ObjectMapParam format_parameter(format_string.begin(),
                                             format_string.end());
  Clipboard::ObjectMapParam data_parameter;

  data_parameter.resize(pickle.size());
  memcpy(const_cast<char*>(&data_parameter.front()),
         pickle.data(), pickle.size());

  Clipboard::ObjectMapParams parameters;
  parameters.push_back(format_parameter);
  parameters.push_back(data_parameter);
  objects_[Clipboard::CBF_DATA] = parameters;
}

void ScopedClipboardWriter::Reset() {
  objects_.clear();
  bitmap_.reset();
}

}  // namespace ui
