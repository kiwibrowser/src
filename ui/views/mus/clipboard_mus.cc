// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/clipboard_mus.h"

#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "mojo/public/cpp/bindings/sync_call_restrictions.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/custom_data_helper.h"
#include "ui/gfx/codec/png_codec.h"

namespace views {
namespace {

ui::mojom::Clipboard::Type GetType(ui::ClipboardType type) {
  switch (type) {
    case ui::CLIPBOARD_TYPE_COPY_PASTE:
      return ui::mojom::Clipboard::Type::COPY_PASTE;
    case ui::CLIPBOARD_TYPE_SELECTION:
      return ui::mojom::Clipboard::Type::SELECTION;
    case ui::CLIPBOARD_TYPE_DRAG:
      // Only OSX uses a drag clipboard.
      break;
  }

  NOTREACHED();
  return ui::mojom::Clipboard::Type::COPY_PASTE;
}

// The source URL of copied HTML.
const char kInternalSourceURL[] = "chromium/internal-url";

}  // namespace

ClipboardMus::ClipboardMus() {}

ClipboardMus::~ClipboardMus() {}

void ClipboardMus::Init(service_manager::Connector* connector) {
  connector->BindInterface(ui::mojom::kServiceName, &clipboard_);
}

// TODO(erg): This isn't optimal. It would be better to move the entire
// FormatType system to mime types throughout chrome, but that's a very large
// change.
std::string ClipboardMus::GetMimeTypeFor(const FormatType& format) {
  if (format.Equals(GetUrlFormatType()) || format.Equals(GetUrlWFormatType()))
    return ui::mojom::kMimeTypeURIList;
  if (format.Equals(GetMozUrlFormatType()))
    return ui::mojom::kMimeTypeMozillaURL;
  if (format.Equals(GetPlainTextFormatType()) ||
      format.Equals(GetPlainTextWFormatType())) {
    return ui::mojom::kMimeTypeText;
  }
  if (format.Equals(GetHtmlFormatType()))
    return ui::mojom::kMimeTypeHTML;
  if (format.Equals(GetRtfFormatType()))
    return ui::mojom::kMimeTypeRTF;
  if (format.Equals(GetBitmapFormatType()))
    return ui::mojom::kMimeTypePNG;
  if (format.Equals(GetWebKitSmartPasteFormatType()))
    return kMimeTypeWebkitSmartPaste;
  if (format.Equals(GetWebCustomDataFormatType()))
    return kMimeTypeWebCustomData;
  if (format.Equals(GetPepperCustomDataFormatType()))
    return kMimeTypePepperCustomData;

  // TODO(erg): This isn't optimal, but it's the best we can do. On windows,
  // this will return strings that aren't MIME types, though they'll be
  // unique and should be serializable on the other side of the mojo
  // connection.
  return format.Serialize();
}

bool ClipboardMus::HasMimeType(const std::vector<std::string>& available_types,
                               const std::string& type) const {
  return base::ContainsValue(available_types, type);
}

void ClipboardMus::OnPreShutdown() {}

uint64_t ClipboardMus::GetSequenceNumber(ui::ClipboardType type) const {
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  uint64_t sequence_number = 0;
  clipboard_->GetSequenceNumber(GetType(type), &sequence_number);
  return sequence_number;
}

bool ClipboardMus::IsFormatAvailable(const FormatType& format,
                                     ui::ClipboardType type) const {
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;

  uint64_t sequence_number = 0;
  std::vector<std::string> available_types;
  clipboard_->GetAvailableMimeTypes(GetType(type), &sequence_number,
                                    &available_types);

  std::string format_in_mime = GetMimeTypeFor(format);
  return base::ContainsValue(available_types, format_in_mime);
}

void ClipboardMus::Clear(ui::ClipboardType type) {
  // Sends the data to mus server.
  uint64_t sequence_number = 0;
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  clipboard_->WriteClipboardData(GetType(type), base::nullopt,
                                 &sequence_number);
}

void ClipboardMus::ReadAvailableTypes(ui::ClipboardType type,
                                      std::vector<base::string16>* types,
                                      bool* contains_filenames) const {
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;

  uint64_t sequence_number = 0;
  std::vector<std::string> available_types;
  clipboard_->GetAvailableMimeTypes(GetType(type), &sequence_number,
                                    &available_types);

  types->clear();
  if (HasMimeType(available_types, ui::mojom::kMimeTypeText))
    types->push_back(base::UTF8ToUTF16(ui::mojom::kMimeTypeText));
  if (HasMimeType(available_types, ui::mojom::kMimeTypeHTML))
    types->push_back(base::UTF8ToUTF16(ui::mojom::kMimeTypeHTML));
  if (HasMimeType(available_types, ui::mojom::kMimeTypeRTF))
    types->push_back(base::UTF8ToUTF16(ui::mojom::kMimeTypeRTF));
  if (HasMimeType(available_types, ui::mojom::kMimeTypePNG))
    types->push_back(base::UTF8ToUTF16(ui::mojom::kMimeTypePNG));

  if (HasMimeType(available_types, kMimeTypeWebCustomData)) {
    base::Optional<std::vector<uint8_t>> custom_data;
    uint64_t sequence_number = 0;
    if (clipboard_->ReadClipboardData(GetType(type), kMimeTypeWebCustomData,
                                      &sequence_number, &custom_data) &&
        custom_data.has_value()) {
      ui::ReadCustomDataTypes(&custom_data->front(), custom_data->size(),
                              types);
    }
  }

  *contains_filenames = false;
}

void ClipboardMus::ReadText(ui::ClipboardType type,
                            base::string16* result) const {
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  base::Optional<std::vector<uint8_t>> text_data;
  uint64_t sequence_number = 0;
  if (clipboard_->ReadClipboardData(GetType(type), ui::mojom::kMimeTypeText,
                                    &sequence_number, &text_data) &&
      text_data) {
    *result = base::UTF8ToUTF16(base::StringPiece(
        reinterpret_cast<char*>(text_data->data()), text_data->size()));
  }
}

void ClipboardMus::ReadAsciiText(ui::ClipboardType type,
                                 std::string* result) const {
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  base::Optional<std::vector<uint8_t>> text_data;
  uint64_t sequence_number = 0;
  if (clipboard_->ReadClipboardData(GetType(type), ui::mojom::kMimeTypeText,
                                    &sequence_number, &text_data) &&
      text_data) {
    result->assign(text_data->begin(), text_data->end());
  }
}

void ClipboardMus::ReadHTML(ui::ClipboardType type,
                            base::string16* markup,
                            std::string* src_url,
                            uint32_t* fragment_start,
                            uint32_t* fragment_end) const {
  markup->clear();
  if (src_url)
    src_url->clear();
  *fragment_start = 0;
  *fragment_end = 0;

  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  base::Optional<std::vector<uint8_t>> html_data;
  uint64_t sequence_number = 0;
  if (clipboard_->ReadClipboardData(GetType(type), ui::mojom::kMimeTypeHTML,
                                    &sequence_number, &html_data) &&
      html_data) {
    *markup = base::UTF8ToUTF16(base::StringPiece(
        reinterpret_cast<char*>(html_data->data()), html_data->size()));
    *fragment_end = static_cast<uint32_t>(markup->length());

    // We only bother fetching the source url if we were the ones who wrote
    // this html data to the clipboard.
    base::Optional<std::vector<uint8_t>> url_data;
    if (clipboard_->ReadClipboardData(GetType(type), kInternalSourceURL,
                                      &sequence_number, &url_data) &&
        url_data) {
      src_url->assign(url_data->begin(), url_data->end());
    }
  }
}

void ClipboardMus::ReadRTF(ui::ClipboardType type, std::string* result) const {
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  base::Optional<std::vector<uint8_t>> rtf_data;
  uint64_t sequence_number = 0;
  if (clipboard_->ReadClipboardData(GetType(type), ui::mojom::kMimeTypeRTF,
                                    &sequence_number, &rtf_data) &&
      rtf_data) {
    result->assign(rtf_data->begin(), rtf_data->end());
  }
}

SkBitmap ClipboardMus::ReadImage(ui::ClipboardType type) const {
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  base::Optional<std::vector<uint8_t>> data;
  uint64_t sequence_number = 0;
  if (clipboard_->ReadClipboardData(GetType(type), ui::mojom::kMimeTypePNG,
                                    &sequence_number, &data) &&
      data.has_value()) {
    SkBitmap bitmap;
    if (gfx::PNGCodec::Decode(&data->front(), data->size(), &bitmap))
      return SkBitmap(bitmap);
  }

  return SkBitmap();
}

void ClipboardMus::ReadCustomData(ui::ClipboardType clipboard_type,
                                  const base::string16& type,
                                  base::string16* result) const {
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  base::Optional<std::vector<uint8_t>> custom_data;
  uint64_t sequence_number = 0;
  if (clipboard_->ReadClipboardData(GetType(clipboard_type),
                                    kMimeTypeWebCustomData, &sequence_number,
                                    &custom_data) &&
      custom_data.has_value()) {
    ui::ReadCustomDataForType(&custom_data->front(), custom_data->size(), type,
                              result);
  }
}

void ClipboardMus::ReadBookmark(base::string16* title, std::string* url) const {
  // TODO(erg): This is NOTIMPLEMENTED() on all linux platforms?
  NOTIMPLEMENTED();
}

void ClipboardMus::ReadData(const FormatType& format,
                            std::string* result) const {
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  base::Optional<std::vector<uint8_t>> data;
  uint64_t sequence_number = 0;
  if (clipboard_->ReadClipboardData(ui::mojom::Clipboard::Type::COPY_PASTE,
                                    GetMimeTypeFor(format), &sequence_number,
                                    &data) &&
      data) {
    result->assign(data->begin(), data->end());
  }
}

void ClipboardMus::WriteObjects(ui::ClipboardType type,
                                const ObjectMap& objects) {
  current_clipboard_.emplace();
  for (const auto& p : objects)
    DispatchObject(static_cast<ObjectType>(p.first), p.second);

  // Sends the data to mus server.
  uint64_t sequence_number = 0;
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  clipboard_->WriteClipboardData(GetType(type), std::move(current_clipboard_),
                                 &sequence_number);
}

void ClipboardMus::WriteText(const char* text_data, size_t text_len) {
  DCHECK(current_clipboard_);
  current_clipboard_.value()[ui::mojom::kMimeTypeText] =
      std::vector<uint8_t>(text_data, text_data + text_len);
}

void ClipboardMus::WriteHTML(const char* markup_data,
                             size_t markup_len,
                             const char* url_data,
                             size_t url_len) {
  DCHECK(current_clipboard_);
  current_clipboard_.value()[ui::mojom::kMimeTypeHTML] =
      std::vector<uint8_t>(markup_data, markup_data + markup_len);
  if (url_len > 0) {
    current_clipboard_.value()[kInternalSourceURL] =
        std::vector<uint8_t>(url_data, url_data + url_len);
  }
}

void ClipboardMus::WriteRTF(const char* rtf_data, size_t data_len) {
  DCHECK(current_clipboard_);
  current_clipboard_.value()[ui::mojom::kMimeTypeRTF] =
      std::vector<uint8_t>(rtf_data, rtf_data + data_len);
}

void ClipboardMus::WriteBookmark(const char* title_data,
                                 size_t title_len,
                                 const char* url_data,
                                 size_t url_len) {
  // Writes a Mozilla url (UTF16: URL, newline, title)
  base::string16 bookmark =
      base::UTF8ToUTF16(base::StringPiece(url_data, url_len)) +
      base::ASCIIToUTF16("\n") +
      base::UTF8ToUTF16(base::StringPiece(title_data, title_len));

  DCHECK(current_clipboard_);
  current_clipboard_.value()[ui::mojom::kMimeTypeMozillaURL] =
      std::vector<uint8_t>(
          reinterpret_cast<const uint8_t*>(bookmark.data()),
          reinterpret_cast<const uint8_t*>(bookmark.data() + bookmark.size()));
}

void ClipboardMus::WriteWebSmartPaste() {
  DCHECK(current_clipboard_);
  current_clipboard_.value()[kMimeTypeWebkitSmartPaste] =
      std::vector<uint8_t>();
}

void ClipboardMus::WriteBitmap(const SkBitmap& bitmap) {
  DCHECK(current_clipboard_);
  // Encode the bitmap as a PNG for transport.
  std::vector<unsigned char> output;
  if (gfx::PNGCodec::FastEncodeBGRASkBitmap(bitmap, false, &output)) {
    current_clipboard_.value()[ui::mojom::kMimeTypePNG] = std::move(output);
  }
}

void ClipboardMus::WriteData(const FormatType& format,
                             const char* data_data,
                             size_t data_len) {
  DCHECK(current_clipboard_);
  current_clipboard_.value()[GetMimeTypeFor(format)] =
      std::vector<uint8_t>(data_data, data_data + data_len);
}

}  // namespace views
