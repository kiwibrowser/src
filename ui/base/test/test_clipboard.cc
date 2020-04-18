// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/test/test_clipboard.h"

#include <stddef.h>
#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/clipboard/clipboard_monitor.h"

namespace ui {

TestClipboard::TestClipboard()
    : default_store_type_(CLIPBOARD_TYPE_COPY_PASTE) {
}

TestClipboard::~TestClipboard() {
}

Clipboard* TestClipboard::CreateForCurrentThread() {
  base::AutoLock lock(Clipboard::clipboard_map_lock_.Get());
  Clipboard* clipboard = new TestClipboard;
  Clipboard::clipboard_map_.Get()[base::PlatformThread::CurrentId()] =
      base::WrapUnique(clipboard);
  return clipboard;
}

void TestClipboard::SetLastModifiedTime(const base::Time& time) {
  last_modified_time_ = time;
}

void TestClipboard::OnPreShutdown() {}

uint64_t TestClipboard::GetSequenceNumber(ClipboardType type) const {
  return GetStore(type).sequence_number;
}

bool TestClipboard::IsFormatAvailable(const FormatType& format,
                                      ClipboardType type) const {
  const DataStore& store = GetStore(type);
  return store.data.find(format) != store.data.end();
}

void TestClipboard::Clear(ClipboardType type) {
  GetStore(type).Clear();
}

void TestClipboard::ReadAvailableTypes(ClipboardType type,
                                       std::vector<base::string16>* types,
                                       bool* contains_filenames) const {
  types->clear();

  if (IsFormatAvailable(Clipboard::GetPlainTextFormatType(), type))
    types->push_back(base::UTF8ToUTF16(kMimeTypeText));
  if (IsFormatAvailable(Clipboard::GetHtmlFormatType(), type))
    types->push_back(base::UTF8ToUTF16(kMimeTypeHTML));

  if (IsFormatAvailable(Clipboard::GetRtfFormatType(), type))
    types->push_back(base::UTF8ToUTF16(kMimeTypeRTF));
  if (IsFormatAvailable(Clipboard::GetBitmapFormatType(), type))
    types->push_back(base::UTF8ToUTF16(kMimeTypePNG));

  *contains_filenames = false;
}

void TestClipboard::ReadText(ClipboardType type, base::string16* result) const {
  std::string result8;
  ReadAsciiText(type, &result8);
  *result = base::UTF8ToUTF16(result8);
}

void TestClipboard::ReadAsciiText(ClipboardType type,
                                  std::string* result) const {
  result->clear();
  const DataStore& store = GetStore(type);
  auto it = store.data.find(GetPlainTextFormatType());
  if (it != store.data.end())
    *result = it->second;
}

void TestClipboard::ReadHTML(ClipboardType type,
                             base::string16* markup,
                             std::string* src_url,
                             uint32_t* fragment_start,
                             uint32_t* fragment_end) const {
  markup->clear();
  src_url->clear();
  const DataStore& store = GetStore(type);
  auto it = store.data.find(GetHtmlFormatType());
  if (it != store.data.end())
    *markup = base::UTF8ToUTF16(it->second);
  *src_url = store.html_src_url;
  *fragment_start = 0;
  *fragment_end = base::checked_cast<uint32_t>(markup->size());
}

void TestClipboard::ReadRTF(ClipboardType type, std::string* result) const {
  result->clear();
  const DataStore& store = GetStore(type);
  auto it = store.data.find(GetRtfFormatType());
  if (it != store.data.end())
    *result = it->second;
}

SkBitmap TestClipboard::ReadImage(ClipboardType type) const {
  return GetStore(type).image;
}

void TestClipboard::ReadCustomData(ClipboardType clipboard_type,
                                   const base::string16& type,
                                   base::string16* result) const {
}

void TestClipboard::ReadBookmark(base::string16* title,
                                 std::string* url) const {
  const DataStore& store = GetDefaultStore();
  auto it = store.data.find(GetUrlWFormatType());
  if (it != store.data.end())
    *url = it->second;
  *title = base::UTF8ToUTF16(store.url_title);
}

void TestClipboard::ReadData(const FormatType& format,
                             std::string* result) const {
  result->clear();
  const DataStore& store = GetDefaultStore();
  auto it = store.data.find(format);
  if (it != store.data.end())
    *result = it->second;
}

base::Time TestClipboard::GetLastModifiedTime() const {
  return last_modified_time_;
}

void TestClipboard::ClearLastModifiedTime() {
  last_modified_time_ = base::Time();
}

void TestClipboard::WriteObjects(ClipboardType type, const ObjectMap& objects) {
  Clear(type);
  default_store_type_ = type;
  for (const auto& kv : objects)
    DispatchObject(static_cast<ObjectType>(kv.first), kv.second);
  default_store_type_ = CLIPBOARD_TYPE_COPY_PASTE;
}

void TestClipboard::WriteText(const char* text_data, size_t text_len) {
  std::string text(text_data, text_len);
  GetDefaultStore().data[GetPlainTextFormatType()] = text;
  // Create a dummy entry.
  GetDefaultStore().data[GetPlainTextWFormatType()];
  if (IsSupportedClipboardType(CLIPBOARD_TYPE_SELECTION))
    GetStore(CLIPBOARD_TYPE_SELECTION).data[GetPlainTextFormatType()] = text;
  ui::ClipboardMonitor::GetInstance()->NotifyClipboardDataChanged();
}

void TestClipboard::WriteHTML(const char* markup_data,
                              size_t markup_len,
                              const char* url_data,
                              size_t url_len) {
  base::string16 markup;
  base::UTF8ToUTF16(markup_data, markup_len, &markup);
  GetDefaultStore().data[GetHtmlFormatType()] = base::UTF16ToUTF8(markup);
  GetDefaultStore().html_src_url = std::string(url_data, url_len);
}

void TestClipboard::WriteRTF(const char* rtf_data, size_t data_len) {
  GetDefaultStore().data[GetRtfFormatType()] = std::string(rtf_data, data_len);
}

void TestClipboard::WriteBookmark(const char* title_data,
                                  size_t title_len,
                                  const char* url_data,
                                  size_t url_len) {
  GetDefaultStore().data[GetUrlWFormatType()] = std::string(url_data, url_len);
  GetDefaultStore().url_title = std::string(title_data, title_len);
}

void TestClipboard::WriteWebSmartPaste() {
  // Create a dummy entry.
  GetDefaultStore().data[GetWebKitSmartPasteFormatType()];
}

void TestClipboard::WriteBitmap(const SkBitmap& bitmap) {
  // Create a dummy entry.
  GetDefaultStore().data[GetBitmapFormatType()];
  SkBitmap& dst = GetDefaultStore().image;
  if (dst.tryAllocPixels(bitmap.info())) {
    bitmap.readPixels(dst.info(), dst.getPixels(), dst.rowBytes(), 0, 0);
  }
}

void TestClipboard::WriteData(const FormatType& format,
                              const char* data_data,
                              size_t data_len) {
  GetDefaultStore().data[format] = std::string(data_data, data_len);
}

TestClipboard::DataStore::DataStore() : sequence_number(0) {
}

TestClipboard::DataStore::DataStore(const DataStore& other) = default;

TestClipboard::DataStore::~DataStore() {
}

void TestClipboard::DataStore::Clear() {
  data.clear();
  url_title.clear();
  html_src_url.clear();
  image = SkBitmap();
}

const TestClipboard::DataStore& TestClipboard::GetStore(
    ClipboardType type) const {
  CHECK(IsSupportedClipboardType(type));
  return stores_[type];
}

TestClipboard::DataStore& TestClipboard::GetStore(ClipboardType type) {
  CHECK(IsSupportedClipboardType(type));
  DataStore& store = stores_[type];
  ++store.sequence_number;
  return store;
}

const TestClipboard::DataStore& TestClipboard::GetDefaultStore() const {
  return GetStore(default_store_type_);
}

TestClipboard::DataStore& TestClipboard::GetDefaultStore() {
  return GetStore(default_store_type_);
}

}  // namespace ui
