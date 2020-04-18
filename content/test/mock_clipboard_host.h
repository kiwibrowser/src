// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_CLIPBOARD_HOST_H_
#define CONTENT_TEST_MOCK_CLIPBOARD_HOST_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/blink/public/mojom/clipboard/clipboard.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace content {

class BrowserContext;

class MockClipboardHost : public blink::mojom::ClipboardHost {
 public:
  // |browser_context| could be null, in which case reading images
  // is not supported.
  explicit MockClipboardHost(BrowserContext* browser_context);
  ~MockClipboardHost() override;

  void Bind(blink::mojom::ClipboardHostRequest request);
  void Reset();

 private:
  // blink::mojom::ClipboardHost
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

  BrowserContext* browser_context_;
  mojo::BindingSet<blink::mojom::ClipboardHost> bindings_;
  uint64_t sequence_number_ = 0;
  base::string16 plain_text_;
  base::string16 html_text_;
  GURL url_;
  SkBitmap image_;
  std::map<base::string16, base::string16> custom_data_;
  bool write_smart_paste_ = false;
  bool needs_reset_ = false;

  DISALLOW_COPY_AND_ASSIGN(MockClipboardHost);
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_CLIPBOARD_HOST_H_
