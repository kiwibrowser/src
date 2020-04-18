// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_clipboard_host.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/blob_handle.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/codec/png_codec.h"

namespace content {

namespace {

void ReleaseSharedMemoryPixels(void* addr, void* context) {
  MojoResult result = MojoUnmapBuffer(context);
  DCHECK_EQ(MOJO_RESULT_OK, result);
}

blink::mojom::SerializedBlobPtr ConstructSerializedBlobOnIO(
    size_t data_size,
    std::unique_ptr<BlobHandle> blob_handle) {
  blink::mojom::SerializedBlobPtr blob;
  if (blob_handle) {
    blob = blink::mojom::SerializedBlob::New(
        blob_handle->GetUUID(), "image/png", static_cast<int64_t>(data_size),
        blob_handle->PassBlob().PassInterface());
  }
  return blob;
}

void ReplyToReadImage(blink::mojom::ClipboardHost::ReadImageCallback callback,
                      size_t data_size,
                      std::vector<unsigned char> owner_buffer,
                      std::unique_ptr<BlobHandle> blob_handle) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Unfortunately, we cannot call blob_handle->PassBlob() on UI,
  // since that binds a blob ptr which only works on IO. If we do,
  // future access to that blob will be handled by Mojo on the wrong
  // thread.
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&ConstructSerializedBlobOnIO, data_size,
                     std::move(blob_handle)),
      std::move(callback));
}

}  // namespace

MockClipboardHost::MockClipboardHost(BrowserContext* browser_context)
    : browser_context_(browser_context) {}

MockClipboardHost::~MockClipboardHost() {}

void MockClipboardHost::Bind(blink::mojom::ClipboardHostRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void MockClipboardHost::Reset() {
  plain_text_ = base::string16();
  html_text_ = base::string16();
  url_ = GURL();
  image_.reset();
  custom_data_.clear();
  write_smart_paste_ = false;
  needs_reset_ = false;
}

void MockClipboardHost::GetSequenceNumber(ui::ClipboardType clipboard_type,
                                          GetSequenceNumberCallback callback) {
  std::move(callback).Run(sequence_number_);
}

void MockClipboardHost::ReadAvailableTypes(
    ui::ClipboardType clipboard_type,
    ReadAvailableTypesCallback callback) {
  std::vector<base::string16> types;
  if (!plain_text_.empty())
    types.push_back(base::UTF8ToUTF16("text/plain"));
  if (!html_text_.empty())
    types.push_back(base::UTF8ToUTF16("text/html"));
  if (!image_.isNull())
    types.push_back(base::UTF8ToUTF16("image/png"));
  for (auto& it : custom_data_) {
    CHECK(std::find(types.begin(), types.end(), it.first) == types.end());
    types.push_back(it.first);
  }
  std::move(callback).Run(types, false);
}

void MockClipboardHost::IsFormatAvailable(blink::mojom::ClipboardFormat format,
                                          ui::ClipboardType clipboard_type,
                                          IsFormatAvailableCallback callback) {
  bool result = false;
  switch (format) {
    case blink::mojom::ClipboardFormat::kPlaintext:
      result = !plain_text_.empty();
      break;
    case blink::mojom::ClipboardFormat::kHtml:
      result = !html_text_.empty();
      break;
    case blink::mojom::ClipboardFormat::kSmartPaste:
      result = write_smart_paste_;
      break;
    case blink::mojom::ClipboardFormat::kBookmark:
      result = false;
      break;
  }
  std::move(callback).Run(result);
}

void MockClipboardHost::ReadText(ui::ClipboardType clipboard_type,
                                 ReadTextCallback callback) {
  std::move(callback).Run(plain_text_);
}

void MockClipboardHost::ReadHtml(ui::ClipboardType clipboard_type,
                                 ReadHtmlCallback callback) {
  std::move(callback).Run(html_text_, url_, 0, html_text_.length());
}

void MockClipboardHost::ReadRtf(ui::ClipboardType clipboard_type,
                                ReadRtfCallback callback) {
  std::move(callback).Run(std::string());
}

void MockClipboardHost::ReadImage(ui::ClipboardType clipboard_type,
                                  ReadImageCallback callback) {
  if (image_.isNull() || !browser_context_) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::vector<unsigned char> png_data;
  if (!gfx::PNGCodec::FastEncodeBGRASkBitmap(image_, false, &png_data)) {
    std::move(callback).Run(nullptr);
    return;
  }
  if (png_data.size() >= std::numeric_limits<uint32_t>::max()) {
    std::move(callback).Run(nullptr);
    return;
  }

  char* char_data = reinterpret_cast<char*>(png_data.data());
  size_t size = png_data.size();
  // We pass png_data to retain encoded data until CreateMemoryBackedBlob
  // takes a copy of it.
  BrowserContext::CreateMemoryBackedBlob(
      browser_context_, char_data, size, "",
      base::BindOnce(&ReplyToReadImage, std::move(callback), size,
                     std::move(png_data)));
}

void MockClipboardHost::ReadCustomData(ui::ClipboardType clipboard_type,
                                       const base::string16& type,
                                       ReadCustomDataCallback callback) {
  auto it = custom_data_.find(type);
  std::move(callback).Run(it != custom_data_.end() ? it->second
                                                   : base::string16());
}

void MockClipboardHost::WriteText(ui::ClipboardType,
                                  const base::string16& text) {
  if (needs_reset_)
    Reset();
  plain_text_ = text;
}

void MockClipboardHost::WriteHtml(ui::ClipboardType,
                                  const base::string16& markup,
                                  const GURL& url) {
  if (needs_reset_)
    Reset();
  html_text_ = markup;
  url_ = url;
}

void MockClipboardHost::WriteSmartPasteMarker(ui::ClipboardType) {
  if (needs_reset_)
    Reset();
  write_smart_paste_ = true;
}

void MockClipboardHost::WriteCustomData(
    ui::ClipboardType,
    const base::flat_map<base::string16, base::string16>& data) {
  if (needs_reset_)
    Reset();
  for (auto& it : data)
    custom_data_[it.first] = it.second;
}

void MockClipboardHost::WriteBookmark(ui::ClipboardType,
                                      const std::string& url,
                                      const base::string16& title) {}

void MockClipboardHost::WriteImage(
    ui::ClipboardType,
    const gfx::Size& size,
    mojo::ScopedSharedBufferHandle shared_buffer_handle) {
  if (needs_reset_)
    Reset();
  if (!image_.setInfo(SkImageInfo::MakeN32Premul(size.width(), size.height())))
    return;
  auto mapped = shared_buffer_handle->Map(image_.computeByteSize());
  if (!mapped)
    return;
  if (!image_.installPixels(image_.info(), mapped.get(), image_.rowBytes(),
                            &ReleaseSharedMemoryPixels, mapped.get())) {
    return;
  }
  mapped.release();
}

void MockClipboardHost::CommitWrite(ui::ClipboardType) {
  ++sequence_number_;
  needs_reset_ = true;
}

#if defined(OS_MACOSX)
void MockClipboardHost::WriteStringToFindPboard(const base::string16& text) {}
#endif

}  // namespace content
