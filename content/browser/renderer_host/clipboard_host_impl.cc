// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/clipboard_host_impl.h"

#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/pickle.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/public/browser/blob_handle.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "ipc/ipc_message_macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/custom_data_helper.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace content {
namespace {

void ReleaseSharedMemoryPixels(void* addr, void* context) {
  MojoResult result = MojoUnmapBuffer(context);
  DCHECK_EQ(MOJO_RESULT_OK, result);
}

void OnReadAndEncodeImageFinished(
    scoped_refptr<ChromeBlobStorageContext> blob_storage_context,
    std::vector<uint8_t> png_data,
    ClipboardHostImpl::ReadImageCallback callback) {
  // |blob_storage_context| must be accessed only on the IO thread.
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  blink::mojom::SerializedBlobPtr blob;
  if (png_data.size() < std::numeric_limits<uint32_t>::max()) {
    std::unique_ptr<content::BlobHandle> blob_handle =
        blob_storage_context->CreateMemoryBackedBlob(
            reinterpret_cast<char*>(png_data.data()), png_data.size(), "");
    if (blob_handle) {
      std::string blob_uuid = blob_handle->GetUUID();
      blob = blink::mojom::SerializedBlob::New(
          blob_uuid, ui::Clipboard::kMimeTypePNG,
          static_cast<int64_t>(png_data.size()),
          blob_handle->PassBlob().PassInterface());
    }
  }
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(std::move(callback), std::move(blob)));
}

void ReadAndEncodeImage(
    scoped_refptr<ChromeBlobStorageContext> blob_storage_context,
    const SkBitmap& bitmap,
    ClipboardHostImpl::ReadImageCallback callback) {
  std::vector<uint8_t> png_data;
  if (!gfx::PNGCodec::FastEncodeBGRASkBitmap(bitmap, false, &png_data)) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(callback), nullptr));
    return;
  }
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&OnReadAndEncodeImageFinished,
                     std::move(blob_storage_context), std::move(png_data),
                     std::move(callback)));
}

}  // namespace

ClipboardHostImpl::ClipboardHostImpl(
    scoped_refptr<ChromeBlobStorageContext> blob_storage_context)
    : clipboard_(ui::Clipboard::GetForCurrentThread()),
      blob_storage_context_(std::move(blob_storage_context)),
      clipboard_writer_(
          new ui::ScopedClipboardWriter(ui::CLIPBOARD_TYPE_COPY_PASTE)) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void ClipboardHostImpl::Create(
    scoped_refptr<ChromeBlobStorageContext> blob_storage_context,
    blink::mojom::ClipboardHostRequest request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  mojo::MakeStrongBinding(
      base::WrapUnique<ClipboardHostImpl>(
          new ClipboardHostImpl(std::move(blob_storage_context))),
      std::move(request));
}

ClipboardHostImpl::~ClipboardHostImpl() {
  clipboard_writer_->Reset();
}

void ClipboardHostImpl::GetSequenceNumber(ui::ClipboardType clipboard_type,
                                          GetSequenceNumberCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::move(callback).Run(clipboard_->GetSequenceNumber(clipboard_type));
}

void ClipboardHostImpl::ReadAvailableTypes(
    ui::ClipboardType clipboard_type,
    ReadAvailableTypesCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::vector<base::string16> types;
  bool contains_filenames;
  clipboard_->ReadAvailableTypes(clipboard_type, &types, &contains_filenames);
  std::move(callback).Run(types, contains_filenames);
}

void ClipboardHostImpl::IsFormatAvailable(blink::mojom::ClipboardFormat format,
                                          ui::ClipboardType clipboard_type,
                                          IsFormatAvailableCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  bool result = false;
  switch (format) {
    case blink::mojom::ClipboardFormat::kPlaintext:
      result = clipboard_->IsFormatAvailable(
                   ui::Clipboard::GetPlainTextWFormatType(), clipboard_type) ||
               clipboard_->IsFormatAvailable(
                   ui::Clipboard::GetPlainTextFormatType(), clipboard_type);
      break;
    case blink::mojom::ClipboardFormat::kHtml:
      result = clipboard_->IsFormatAvailable(ui::Clipboard::GetHtmlFormatType(),
                                             clipboard_type);
      break;
    case blink::mojom::ClipboardFormat::kSmartPaste:
      result = clipboard_->IsFormatAvailable(
          ui::Clipboard::GetWebKitSmartPasteFormatType(), clipboard_type);
      break;
    case blink::mojom::ClipboardFormat::kBookmark:
#if defined(OS_WIN) || defined(OS_MACOSX)
      result = clipboard_->IsFormatAvailable(ui::Clipboard::GetUrlWFormatType(),
                                             clipboard_type);
#else
      result = false;
#endif
      break;
  }
  std::move(callback).Run(result);
}

void ClipboardHostImpl::ReadText(ui::ClipboardType clipboard_type,
                                 ReadTextCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::string16 result;
  if (clipboard_->IsFormatAvailable(ui::Clipboard::GetPlainTextWFormatType(),
                                    clipboard_type)) {
    clipboard_->ReadText(clipboard_type, &result);
  } else if (clipboard_->IsFormatAvailable(
                 ui::Clipboard::GetPlainTextFormatType(), clipboard_type)) {
    std::string ascii;
    clipboard_->ReadAsciiText(clipboard_type, &ascii);
    result = base::ASCIIToUTF16(ascii);
  }
  std::move(callback).Run(result);
}

void ClipboardHostImpl::ReadHtml(ui::ClipboardType clipboard_type,
                                 ReadHtmlCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::string16 markup;
  std::string src_url_str;
  uint32_t fragment_start = 0;
  uint32_t fragment_end = 0;
  clipboard_->ReadHTML(clipboard_type, &markup, &src_url_str, &fragment_start,
                       &fragment_end);
  std::move(callback).Run(std::move(markup), GURL(src_url_str), fragment_start,
                          fragment_end);
}

void ClipboardHostImpl::ReadRtf(ui::ClipboardType clipboard_type,
                                ReadRtfCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::string result;
  clipboard_->ReadRTF(clipboard_type, &result);
  std::move(callback).Run(result);
}

void ClipboardHostImpl::ReadImage(ui::ClipboardType clipboard_type,
                                  ReadImageCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  SkBitmap bitmap = clipboard_->ReadImage(clipboard_type);

  if (bitmap.isNull()) {
    std::move(callback).Run(nullptr);
    return;
  }
  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&ReadAndEncodeImage, blob_storage_context_,
                     std::move(bitmap), std::move(callback)));
}

void ClipboardHostImpl::ReadCustomData(ui::ClipboardType clipboard_type,
                                       const base::string16& type,
                                       ReadCustomDataCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::string16 result;
  clipboard_->ReadCustomData(clipboard_type, type, &result);
  std::move(callback).Run(result);
}

void ClipboardHostImpl::WriteText(ui::ClipboardType,
                                  const base::string16& text) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  clipboard_writer_->WriteText(text);
}

void ClipboardHostImpl::WriteHtml(ui::ClipboardType,
                                  const base::string16& markup,
                                  const GURL& url) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  clipboard_writer_->WriteHTML(markup, url.spec());
}

void ClipboardHostImpl::WriteSmartPasteMarker(ui::ClipboardType) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  clipboard_writer_->WriteWebSmartPaste();
}

void ClipboardHostImpl::WriteCustomData(
    ui::ClipboardType,
    const base::flat_map<base::string16, base::string16>& data) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  base::Pickle pickle;
  ui::WriteCustomDataToPickle(data, &pickle);
  clipboard_writer_->WritePickledData(
      pickle, ui::Clipboard::GetWebCustomDataFormatType());
}

void ClipboardHostImpl::WriteBookmark(ui::ClipboardType,
                                      const std::string& url,
                                      const base::string16& title) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  clipboard_writer_->WriteBookmark(title, url);
}

void ClipboardHostImpl::WriteImage(
    ui::ClipboardType,
    const gfx::Size& size,
    mojo::ScopedSharedBufferHandle shared_buffer_handle) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  SkBitmap bitmap;
  // Let Skia do some sanity checking for (no negative widths/heights, no
  // overflows while calculating bytes per row, etc).
  if (!bitmap.setInfo(
          SkImageInfo::MakeN32Premul(size.width(), size.height()))) {
    return;
  }

  auto mapped = shared_buffer_handle->Map(bitmap.computeByteSize());
  if (!mapped) {
    return;
  }

  if (!bitmap.installPixels(bitmap.info(), mapped.get(), bitmap.rowBytes(),
                            &ReleaseSharedMemoryPixels, mapped.get())) {
    return;
  }

  // On success, SkBitmap now owns the SharedMemory.
  mapped.release();
  clipboard_writer_->WriteImage(bitmap);
}

void ClipboardHostImpl::CommitWrite(ui::ClipboardType) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  clipboard_writer_.reset(
      new ui::ScopedClipboardWriter(ui::CLIPBOARD_TYPE_COPY_PASTE));
}

}  // namespace content
