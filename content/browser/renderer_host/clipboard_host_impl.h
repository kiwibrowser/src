// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_CLIPBOARD_HOST_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_CLIPBOARD_HOST_IMPL_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "build/build_config.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_associated_interface.h"
#include "content/public/browser/browser_message_filter.h"
#include "third_party/blink/public/mojom/clipboard/clipboard.mojom.h"
#include "ui/base/clipboard/clipboard.h"

class GURL;

namespace gfx {
class Size;
}

namespace ui {
class ScopedClipboardWriter;
}  // namespace ui

namespace content {

class ChromeBlobStorageContext;
class ClipboardHostImplTest;

class CONTENT_EXPORT ClipboardHostImpl : public blink::mojom::ClipboardHost {
 public:
  ~ClipboardHostImpl() override;

  static void Create(
      scoped_refptr<ChromeBlobStorageContext> blob_storage_context,
      blink::mojom::ClipboardHostRequest request);

 private:
  friend class ClipboardHostImplTest;

  explicit ClipboardHostImpl(
      scoped_refptr<ChromeBlobStorageContext> blob_storage_context);

  // content::mojom::ClipboardHost
  void GetSequenceNumber(ui::ClipboardType clipboard_type,
                         GetSequenceNumberCallback callback) override;
  void IsFormatAvailable(blink::mojom::ClipboardFormat format,
                         ui::ClipboardType clipboard_type,
                         IsFormatAvailableCallback callback) override;
  void ReadAvailableTypes(ui::ClipboardType clipboard_type,
                          ReadAvailableTypesCallback callback) override;
  void ReadText(ui::ClipboardType clipboard_type,
                ReadTextCallback callback) override;
  void ReadHtml(ui::ClipboardType clipboard_type,
                ReadHtmlCallback callback) override;
  void ReadRtf(ui::ClipboardType clipboard_type,
               ReadRtfCallback callback) override;
  void ReadImage(ui::ClipboardType clipboard_type,
                 ReadImageCallback callback) override;
  void ReadCustomData(ui::ClipboardType clipboard_type,
                      const base::string16& type,
                      ReadCustomDataCallback callback) override;
  void WriteText(ui::ClipboardType clipboard_type,
                 const base::string16& text) override;
  void WriteHtml(ui::ClipboardType clipboard_type,
                 const base::string16& markup,
                 const GURL& url) override;
  void WriteSmartPasteMarker(ui::ClipboardType clipboard_type) override;
  void WriteCustomData(
      ui::ClipboardType clipboard_type,
      const base::flat_map<base::string16, base::string16>& data) override;
  void WriteBookmark(ui::ClipboardType clipboard_type,
                     const std::string& url,
                     const base::string16& title) override;
  void WriteImage(ui::ClipboardType clipboard_type,
                  const gfx::Size& size_in_pixels,
                  mojo::ScopedSharedBufferHandle shared_buffer_handle) override;
  void CommitWrite(ui::ClipboardType clipboard_type) override;
#if defined(OS_MACOSX)
  void WriteStringToFindPboard(const base::string16& text) override;
#endif

  ui::Clipboard* clipboard_;  // Not owned
  scoped_refptr<ChromeBlobStorageContext> blob_storage_context_;
  std::unique_ptr<ui::ScopedClipboardWriter> clipboard_writer_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_CLIPBOARD_HOST_IMPL_H_
