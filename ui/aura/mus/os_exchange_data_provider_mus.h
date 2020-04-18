// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_OS_EXCHANGE_DATA_PROVIDER_MUS_H_
#define UI_AURA_MUS_OS_EXCHANGE_DATA_PROVIDER_MUS_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ui/aura/aura_export.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/image/image_skia.h"

namespace aura {

// Translates chrome's requests for various data types to a platform specific
// type. In the case of mus, this is a mapping of MIME types to byte arrays.
//
// TODO(erg): In the long run, there's a lot of optimizations that we can do
// once everything targets mus. The entire model of OSExchangeDataProvider
// shoves all data across the wire at once whether it is needed or not.
class AURA_EXPORT OSExchangeDataProviderMus
    : public ui::OSExchangeData::Provider {
 public:
  using Data = std::map<std::string, std::vector<uint8_t>>;

  OSExchangeDataProviderMus();
  explicit OSExchangeDataProviderMus(Data data);
  ~OSExchangeDataProviderMus() override;

  // Returns the raw MIME type to data mapping.
  Data GetData() const;

  // Overridden from OSExchangeData::Provider:
  std::unique_ptr<Provider> Clone() const override;

  void MarkOriginatedFromRenderer() override;
  bool DidOriginateFromRenderer() const override;

  void SetString(const base::string16& data) override;
  void SetURL(const GURL& url, const base::string16& title) override;
  void SetFilename(const base::FilePath& path) override;
  void SetFilenames(const std::vector<ui::FileInfo>& file_names) override;
  void SetPickledData(const ui::Clipboard::FormatType& format,
                      const base::Pickle& data) override;

  bool GetString(base::string16* data) const override;
  bool GetURLAndTitle(ui::OSExchangeData::FilenameToURLPolicy policy,
                      GURL* url,
                      base::string16* title) const override;
  bool GetFilename(base::FilePath* path) const override;
  bool GetFilenames(std::vector<ui::FileInfo>* file_names) const override;
  bool GetPickledData(const ui::Clipboard::FormatType& format,
                      base::Pickle* data) const override;

  bool HasString() const override;
  bool HasURL(ui::OSExchangeData::FilenameToURLPolicy policy) const override;
  bool HasFile() const override;
  bool HasCustomFormat(const ui::Clipboard::FormatType& format) const override;

// Provider doesn't have a consistent interface between operating systems;
// this wasn't seen as a problem when there was a single Provider subclass
// per operating system. Now we have to have at least two providers per OS,
// leading to the following warts, which will remain until we clean all the
// callsites up.
#if defined(USE_X11) || defined(OS_WIN)
  void SetFileContents(const base::FilePath& filename,
                       const std::string& file_contents) override;
#endif
#if defined(OS_WIN)
  bool GetFileContents(base::FilePath* filename,
                       std::string* file_contents) const override;
  bool HasFileContents() const override;
  void SetDownloadFileInfo(
      const ui::OSExchangeData::DownloadFileInfo& download) override;
#endif

#if defined(USE_AURA)
  void SetHtml(const base::string16& html, const GURL& base_url) override;
  bool GetHtml(base::string16* html, GURL* base_url) const override;
  bool HasHtml() const override;
#endif

#if defined(USE_AURA) || defined(OS_MACOSX)
  void SetDragImage(const gfx::ImageSkia& image,
                    const gfx::Vector2d& cursor_offset) override;
  gfx::ImageSkia GetDragImage() const override;
  gfx::Vector2d GetDragImageOffset() const override;
#endif

 private:
  // Returns true if |formats_| contains a file format and the file name can be
  // parsed as a URL.
  bool GetFileURL(GURL* url) const;

  // Returns true if |formats_| contains a string format and the string can be
  // parsed as a URL.
  bool GetPlainTextURL(GURL* url) const;

  // Drag image and offset data.
  gfx::ImageSkia drag_image_;
  gfx::Vector2d drag_image_offset_;

  Data mime_data_;

  DISALLOW_COPY_AND_ASSIGN(OSExchangeDataProviderMus);
};

}  // namespace aura

#endif  // UI_AURA_MUS_OS_EXCHANGE_DATA_PROVIDER_MUS_H_
