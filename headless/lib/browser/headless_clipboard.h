// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_CLIPBOARD_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_CLIPBOARD_H_

#include <stddef.h>
#include <stdint.h>

#include <map>

#include "base/macros.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard.h"

namespace headless {

class HeadlessClipboard : public ui::Clipboard {
 public:
  HeadlessClipboard();
  ~HeadlessClipboard() override;

 private:
  // Clipboard overrides.
  void OnPreShutdown() override;
  uint64_t GetSequenceNumber(ui::ClipboardType type) const override;
  bool IsFormatAvailable(const FormatType& format,
                         ui::ClipboardType type) const override;
  void Clear(ui::ClipboardType type) override;
  void ReadAvailableTypes(ui::ClipboardType type,
                          std::vector<base::string16>* types,
                          bool* contains_filenames) const override;
  void ReadText(ui::ClipboardType type, base::string16* result) const override;
  void ReadAsciiText(ui::ClipboardType type,
                     std::string* result) const override;
  void ReadHTML(ui::ClipboardType type,
                base::string16* markup,
                std::string* src_url,
                uint32_t* fragment_start,
                uint32_t* fragment_end) const override;
  void ReadRTF(ui::ClipboardType type, std::string* result) const override;
  SkBitmap ReadImage(ui::ClipboardType type) const override;
  void ReadCustomData(ui::ClipboardType clipboard_type,
                      const base::string16& type,
                      base::string16* result) const override;
  void ReadBookmark(base::string16* title, std::string* url) const override;
  void ReadData(const FormatType& format, std::string* result) const override;
  void WriteObjects(ui::ClipboardType type, const ObjectMap& objects) override;
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

  struct DataStore {
    DataStore();
    DataStore(const DataStore& other);
    ~DataStore();
    void Clear();
    uint64_t sequence_number;
    std::map<FormatType, std::string> data;
    std::string url_title;
    std::string html_src_url;
    SkBitmap image;
  };

  // The non-const versions increment the sequence number as a side effect.
  const DataStore& GetStore(ui::ClipboardType type) const;
  const DataStore& GetDefaultStore() const;
  DataStore& GetStore(ui::ClipboardType type);
  DataStore& GetDefaultStore();

  ui::ClipboardType default_store_type_;
  mutable std::map<ui::ClipboardType, DataStore> stores_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessClipboard);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_CLIPBOARD_H_
