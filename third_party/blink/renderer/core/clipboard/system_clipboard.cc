// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/clipboard/system_clipboard.h"

#include "base/memory/scoped_refptr.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_drag_data.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/core/clipboard/data_object.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/clipboard/clipboard_mime_types.h"
#include "third_party/blink/renderer/platform/clipboard/clipboard_utilities.h"
#include "third_party/blink/renderer/platform/graphics/image.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/checked_numeric.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace blink {

namespace {

String NonNullString(const String& string) {
  return string.IsNull() ? g_empty_string16_bit : string;
}

}  // namespace

// static
SystemClipboard& SystemClipboard::GetInstance() {
  DEFINE_STATIC_LOCAL(SystemClipboard, clipboard, ());
  return clipboard;
}

SystemClipboard::SystemClipboard() {
  Platform::Current()->GetInterfaceProvider()->GetInterface(
      mojo::MakeRequest(&clipboard_));
}

bool SystemClipboard::IsSelectionMode() const {
  return buffer_ == mojom::ClipboardBuffer::kSelection;
}

void SystemClipboard::SetSelectionMode(bool selection_mode) {
  buffer_ = selection_mode ? mojom::ClipboardBuffer::kSelection
                           : mojom::ClipboardBuffer::kStandard;
}

bool SystemClipboard::CanSmartReplace() {
  if (!IsValidBufferType(buffer_))
    return false;
  bool result = false;
  clipboard_->IsFormatAvailable(mojom::ClipboardFormat::kSmartPaste, buffer_,
                                &result);
  return result;
}

bool SystemClipboard::IsHTMLAvailable() {
  if (!IsValidBufferType(buffer_))
    return false;
  bool result = false;
  clipboard_->IsFormatAvailable(mojom::ClipboardFormat::kHtml, buffer_,
                                &result);
  return result;
}

uint64_t SystemClipboard::SequenceNumber() {
  if (!IsValidBufferType(buffer_))
    return 0;

  uint64_t result = 0;
  clipboard_->GetSequenceNumber(buffer_, &result);
  return result;
}

Vector<String> SystemClipboard::ReadAvailableTypes() {
  Vector<String> types;
  if (IsValidBufferType(buffer_)) {
    bool unused;
    clipboard_->ReadAvailableTypes(buffer_, &types, &unused);
  }
  return types;
}

String SystemClipboard::ReadPlainText() {
  return ReadPlainText(buffer_);
}

String SystemClipboard::ReadPlainText(mojom::ClipboardBuffer buffer) {
  if (!IsValidBufferType(buffer))
    return String();
  String text;
  clipboard_->ReadText(buffer, &text);
  return text;
}

void SystemClipboard::WritePlainText(const String& plain_text,
                                     SmartReplaceOption) {
  // FIXME: add support for smart replace
  String text = plain_text;
#if defined(OS_WIN)
  ReplaceNewlinesWithWindowsStyleNewlines(text);
#endif
  clipboard_->WriteText(mojom::ClipboardBuffer::kStandard, NonNullString(text));
  clipboard_->CommitWrite(mojom::ClipboardBuffer::kStandard);
}

String SystemClipboard::ReadHTML(KURL& url,
                                 unsigned& fragment_start,
                                 unsigned& fragment_end) {
  String html;
  if (IsValidBufferType(buffer_)) {
    clipboard_->ReadHtml(buffer_, &html, &url,
                         static_cast<uint32_t*>(&fragment_start),
                         static_cast<uint32_t*>(&fragment_end));
  }
  if (html.IsEmpty()) {
    url = KURL();
    fragment_start = 0;
    fragment_end = 0;
  }
  return html;
}

void SystemClipboard::WriteHTML(const String& markup,
                                const KURL& document_url,
                                const String& plain_text,
                                SmartReplaceOption smart_replace_option) {
  String text = plain_text;
#if defined(OS_WIN)
  ReplaceNewlinesWithWindowsStyleNewlines(text);
#endif
  ReplaceNBSPWithSpace(text);

  clipboard_->WriteHtml(mojom::ClipboardBuffer::kStandard,
                        NonNullString(markup), document_url);
  clipboard_->WriteText(mojom::ClipboardBuffer::kStandard, NonNullString(text));
  if (smart_replace_option == kCanSmartReplace)
    clipboard_->WriteSmartPasteMarker(mojom::ClipboardBuffer::kStandard);
  clipboard_->CommitWrite(mojom::ClipboardBuffer::kStandard);
}

String SystemClipboard::ReadRTF() {
  if (!IsValidBufferType(buffer_))
    return String();
  String rtf;
  clipboard_->ReadRtf(buffer_, &rtf);
  return rtf;
}

scoped_refptr<BlobDataHandle> SystemClipboard::ReadImage(
    mojom::ClipboardBuffer buffer) {
  if (!IsValidBufferType(buffer))
    return nullptr;
  scoped_refptr<BlobDataHandle> blob;
  clipboard_->ReadImage(buffer, &blob);
  return blob;
}

void SystemClipboard::WriteImage(Image* image,
                                 const KURL& url,
                                 const String& title) {
  DCHECK(image);

  PaintImage paint_image = image->PaintImageForCurrentFrame();
  SkBitmap bitmap;
  if (sk_sp<SkImage> sk_image = paint_image.GetSkImage())
    sk_image->asLegacyBitmap(&bitmap);
  if (bitmap.isNull())
    return;

  // Only 32-bit bitmaps are supported.
  DCHECK_EQ(bitmap.colorType(), kN32_SkColorType);
  const WebSize size(bitmap.width(), bitmap.height());
  void* pixels = bitmap.getPixels();
  // TODO(piman): this should not be NULL, but it is. crbug.com/369621
  if (!pixels)
    return;

  CheckedNumeric<uint32_t> checked_buf_size = 4;
  checked_buf_size *= size.width;
  checked_buf_size *= size.height;
  if (!checked_buf_size.IsValid())
    return;

  // Allocate a shared memory buffer to hold the bitmap bits.
  uint32_t buf_size = checked_buf_size.ValueOrDie();
  auto shared_buffer = mojo::SharedBufferHandle::Create(buf_size);
  auto mapping = shared_buffer->Map(buf_size);
  memcpy(mapping.get(), pixels, buf_size);

  clipboard_->WriteImage(mojom::ClipboardBuffer::kStandard, size,
                         std::move(shared_buffer));

  if (url.IsValid() && !url.IsEmpty()) {
    clipboard_->WriteBookmark(mojom::ClipboardBuffer::kStandard,
                              url.GetString(), NonNullString(title));

    // When writing the image, we also write the image markup so that pasting
    // into rich text editors, such as Gmail, reveals the image. We also don't
    // want to call writeText(), since some applications (WordPad) don't pick
    // the image if there is also a text format on the clipboard.
    clipboard_->WriteHtml(mojom::ClipboardBuffer::kStandard,
                          URLToImageMarkup(url, title), KURL());
  }
  clipboard_->CommitWrite(mojom::ClipboardBuffer::kStandard);
}

String SystemClipboard::ReadCustomData(const String& type) {
  if (!IsValidBufferType(buffer_))
    return String();
  String data;
  clipboard_->ReadCustomData(buffer_, NonNullString(type), &data);
  return data;
}

void SystemClipboard::WriteDataObject(DataObject* data_object) {
  // This plagiarizes the logic in DropDataBuilder::Build, but only extracts the
  // data needed for the implementation of WriteDataObject.
  //
  // We avoid calling the WriteFoo functions if there is no data associated with
  // a type. This prevents stomping on clipboard contents that might have been
  // written by extension functions such as chrome.bookmarkManagerPrivate.copy.
  //
  // TODO(slangley): Use a mojo struct to send web_drag_data and allow receiving
  // side to extract the data required.
  // TODO(dcheng): Properly support text/uri-list here.

  HashMap<String, String> custom_data;
  WebDragData data = data_object->ToWebDragData();
  const WebVector<WebDragData::Item>& item_list = data.Items();
  for (size_t i = 0; i < item_list.size(); ++i) {
    const WebDragData::Item& item = item_list[i];
    if (item.storage_type == WebDragData::Item::kStorageTypeString) {
      if (item.string_type == blink::kMimeTypeTextPlain) {
        clipboard_->WriteText(mojom::ClipboardBuffer::kStandard,
                              NonNullString(item.string_data));
      } else if (item.string_type == blink::kMimeTypeTextHTML) {
        clipboard_->WriteHtml(mojom::ClipboardBuffer::kStandard,
                              NonNullString(item.string_data), KURL());
      } else if (item.string_type != blink::kMimeTypeDownloadURL) {
        custom_data.insert(item.string_type, NonNullString(item.string_data));
      }
    }
  }
  if (!custom_data.IsEmpty()) {
    clipboard_->WriteCustomData(mojom::ClipboardBuffer::kStandard,
                                std::move(custom_data));
  }
  clipboard_->CommitWrite(mojom::ClipboardBuffer::kStandard);
}

bool SystemClipboard::IsValidBufferType(mojom::ClipboardBuffer buffer) {
  switch (buffer) {
    case mojom::ClipboardBuffer::kStandard:
      return true;
    case mojom::ClipboardBuffer::kSelection:
#if defined(USE_X11)
      return true;
#else
      // Chrome OS and non-X11 unix builds do not support
      // the X selection clipboad.
      // TODO: remove the need for this case, see http://crbug.com/361753
      return false;
#endif
  }
  return true;
}

}  // namespace blink
