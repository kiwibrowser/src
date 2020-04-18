// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/clipboard_mus.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/views/mus/mus_client.h"
#include "ui/views/test/scoped_views_test_helper.h"

namespace ui {

// So we can't make ScopedViewsTestHelper a global. We must set up
// ScopedViewsTestHelper on every test (which will create the connection to
// mus). And we can't modify PlatformClipboardTraits to not be a pure static
// struct. So to solve these lifetime issues, create an adapter that owns the
// ScopedViewsTestHelper and then .
class ForwardingTestingClipboard : public ui::Clipboard {
 public:
  ForwardingTestingClipboard()
      : test_helper_(new views::ScopedViewsTestHelper),
        clipboard_to_test_(Clipboard::GetForCurrentThread()) {
    // If we don't have a window manager connection, we will get the default
    // platform clipboard instead.
    EXPECT_TRUE(views::MusClient::Exists());
  }

  ~ForwardingTestingClipboard() override {
    Clipboard::DestroyClipboardForCurrentThread();
  }

  void Destroy() {
    delete this;
  }

 protected:
  // Overridden from ui::Clipboard:
  void OnPreShutdown() override {}

  uint64_t GetSequenceNumber(ClipboardType type) const override {
    return clipboard_to_test_->GetSequenceNumber(type);
  }
  bool IsFormatAvailable(const FormatType& format,
                         ClipboardType type) const override {
    return clipboard_to_test_->IsFormatAvailable(format, type);
  }
  void Clear(ClipboardType type) override {
    clipboard_to_test_->Clear(type);
  }
  void ReadAvailableTypes(ClipboardType type,
                          std::vector<base::string16>* types,
                          bool* contains_filenames) const override {
    clipboard_to_test_->ReadAvailableTypes(type, types, contains_filenames);
  }
  void ReadText(ClipboardType type, base::string16* result) const override {
    clipboard_to_test_->ReadText(type, result);
  }
  void ReadAsciiText(ClipboardType type, std::string* result) const override {
    clipboard_to_test_->ReadAsciiText(type, result);
  }
  void ReadHTML(ClipboardType type, base::string16* markup,
                std::string* src_url, uint32_t* fragment_start,
                uint32_t* fragment_end) const override {
    clipboard_to_test_->ReadHTML(type, markup, src_url,
                                 fragment_start, fragment_end);
  }
  void ReadRTF(ClipboardType type, std::string* result) const override {
    clipboard_to_test_->ReadRTF(type, result);
  }
  SkBitmap ReadImage(ClipboardType type) const override {
    return clipboard_to_test_->ReadImage(type);
  }
  void ReadCustomData(ClipboardType clipboard_type,
                      const base::string16& type,
                      base::string16* result) const override {
    clipboard_to_test_->ReadCustomData(clipboard_type, type, result);
  }
  void ReadBookmark(base::string16* title, std::string* url) const override {
    clipboard_to_test_->ReadBookmark(title, url);
  }
  void ReadData(const FormatType& format, std::string* result) const override {
    clipboard_to_test_->ReadData(format, result);
  }
  void WriteObjects(ClipboardType type, const ObjectMap& objects) override {
    clipboard_to_test_->WriteObjects(type, objects);
  }
  void WriteText(const char* text_data, size_t text_len) override {
    clipboard_to_test_->WriteText(text_data, text_len);
  }
  void WriteHTML(const char* markup_data,
                 size_t markup_len,
                 const char* url_data,
                 size_t url_len) override {
    clipboard_to_test_->WriteHTML(markup_data, markup_len, url_data, url_len);
  }
  void WriteRTF(const char* rtf_data, size_t data_len) override {
    clipboard_to_test_->WriteRTF(rtf_data, data_len);
  }
  void WriteBookmark(const char* title_data,
                     size_t title_len,
                     const char* url_data,
                     size_t url_len) override {
    clipboard_to_test_->WriteBookmark(title_data, title_len,
                                      url_data, url_len);
  }
  void WriteWebSmartPaste() override {
    clipboard_to_test_->WriteWebSmartPaste();
  }
  void WriteBitmap(const SkBitmap& bitmap) override {
    clipboard_to_test_->WriteBitmap(bitmap);
  }
  void WriteData(const FormatType& format,
                 const char* data_data,
                 size_t data_len) override {
    clipboard_to_test_->WriteData(format, data_data, data_len);
  }

 private:
  std::unique_ptr<views::ScopedViewsTestHelper> test_helper_;
  ui::Clipboard* clipboard_to_test_;

  DISALLOW_COPY_AND_ASSIGN(ForwardingTestingClipboard);
};

struct PlatformClipboardTraits {
  static std::unique_ptr<PlatformEventSource> GetEventSource() {
    return nullptr;
  }

  static Clipboard* Create() {
    return new ForwardingTestingClipboard();
  }

  static bool IsMusTest() { return true; }

  static void Destroy(Clipboard* clipboard) {
    static_cast<ForwardingTestingClipboard*>(clipboard)->Destroy();
  }
};

using TypesToTest = PlatformClipboardTraits;

}  // namespace ui

#include "ui/base/clipboard/clipboard_test_template.h"
