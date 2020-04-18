// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_TEST_TEST_CLIPBOARD_H_
#define UI_BASE_TEST_TEST_CLIPBOARD_H_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard.h"

namespace ui {

class TestClipboard : public Clipboard {
 public:
  TestClipboard();
  ~TestClipboard() override;

  // Creates and associates a TestClipboard with the current thread. When no
  // longer needed, the returned clipboard must be freed by calling
  // Clipboard::DestroyClipboardForCurrentThread() on the same thread.
  static Clipboard* CreateForCurrentThread();

  // Sets the time to be returned by GetLastModifiedTime();
  void SetLastModifiedTime(const base::Time& time);

  // Clipboard overrides.
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
  base::Time GetLastModifiedTime() const override;
  void ClearLastModifiedTime() override;
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

 private:
  struct DataStore {
    DataStore();
    DataStore(const DataStore& other);
    ~DataStore();
    void Clear();
    uint64_t sequence_number;
    base::flat_map<FormatType, std::string> data;
    std::string url_title;
    std::string html_src_url;
    SkBitmap image;
  };

  // The non-const versions increment the sequence number as a side effect.
  const DataStore& GetStore(ClipboardType type) const;
  const DataStore& GetDefaultStore() const;
  DataStore& GetStore(ClipboardType type);
  DataStore& GetDefaultStore();

  ClipboardType default_store_type_;
  mutable base::flat_map<ClipboardType, DataStore> stores_;
  base::Time last_modified_time_;

  DISALLOW_COPY_AND_ASSIGN(TestClipboard);
};

}  // namespace ui

#endif  // UI_BASE_TEST_TEST_CLIPBOARD_H_
