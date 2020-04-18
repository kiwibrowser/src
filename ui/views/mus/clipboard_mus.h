// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_CLIPBOARD_MUS_H_
#define UI_VIEWS_MUS_CLIPBOARD_MUS_H_

#include "base/containers/flat_map.h"
#include "services/ui/public/interfaces/clipboard.mojom.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/views/mus/mus_export.h"

namespace service_manager {
class Connector;
}

namespace views {

// An adaptor class which translates the ui::Clipboard interface to the
// clipboard provided by mus.
class VIEWS_MUS_EXPORT ClipboardMus : public ui::Clipboard {
 public:
  ClipboardMus();
  ~ClipboardMus() override;

  void Init(service_manager::Connector* connector);

 private:
  bool HasMimeType(const std::vector<std::string>& available_types,
                   const std::string& type) const;

  // Clipboard overrides:
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

  static std::string GetMimeTypeFor(const FormatType& format);

  ui::mojom::ClipboardPtr clipboard_;

  // Internal buffer used to accumulate data types. The public interface is
  // WriteObjects(), which then calls our base class DispatchObject() which
  // then calls into each data type specific Write() function. Once we've
  // collected all the data types, we then pass this to the mus server.
  base::Optional<base::flat_map<std::string, std::vector<uint8_t>>>
      current_clipboard_;

  DISALLOW_COPY_AND_ASSIGN(ClipboardMus);
};

}  // namespace views

#endif  // UI_VIEWS_MUS_CLIPBOARD_MUS_H_
