// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Note: This header doesn't use REGISTER_TYPED_TEST_CASE_P like most
// type-parameterized gtests. There are lot of test cases in here that are only
// enabled on certain platforms. However, preprocessor directives in macro
// arguments result in undefined behavior (and don't work on MSVC). Instead,
// 'parameterized' tests should typedef TypesToTest (which is used to
// instantiate the tests using the TYPED_TEST_CASE macro) and then #include this
// header.
// TODO(dcheng): This is really horrible. In general, all tests should run on
// all platforms, to avoid this mess.

#include <stdint.h>

#include <memory>
#include <string>

#include "base/pickle.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkScalar.h"
#include "third_party/skia/include/core/SkUnPreMultiply.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/test/test_clipboard.h"
#include "ui/gfx/geometry/size.h"

#if defined(OS_WIN)
#include "ui/base/clipboard/clipboard_util_win.h"
#endif

#if defined(USE_AURA)
#include "ui/events/platform/platform_event_source.h"
#endif

using base::ASCIIToUTF16;
using base::UTF8ToUTF16;
using base::UTF16ToUTF8;

using testing::Contains;

namespace ui {

template <typename ClipboardTraits>
class ClipboardTest : public PlatformTest {
 public:
#if defined(USE_AURA)
  ClipboardTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        event_source_(ClipboardTraits::GetEventSource()),
        clipboard_(ClipboardTraits::Create()) {}
#else
  ClipboardTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        clipboard_(ClipboardTraits::Create()) {}
#endif

  ~ClipboardTest() override { ClipboardTraits::Destroy(clipboard_); }

  bool IsMusTest() { return ClipboardTraits::IsMusTest(); }

 protected:
  Clipboard& clipboard() { return *clipboard_; }

  std::vector<base::string16> GetAvailableTypes(ClipboardType type) {
    bool contains_filenames;
    std::vector<base::string16> types;
    clipboard().ReadAvailableTypes(type, &types, &contains_filenames);
    return types;
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
#if defined(USE_AURA)
  std::unique_ptr<PlatformEventSource> event_source_;
#endif
  // ui::Clipboard has a protected destructor, so scoped_ptr doesn't work here.
  Clipboard* const clipboard_;
};

// Hack for tests that need to call static methods of ClipboardTest.
struct NullClipboardTraits {
  static Clipboard* Create() { return nullptr; }
  static bool IsMusTest() { return false; }
  static void Destroy(Clipboard*) {}
};

TYPED_TEST_CASE(ClipboardTest, TypesToTest);

TYPED_TEST(ClipboardTest, ClearTest) {
  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteText(ASCIIToUTF16("clear me"));
  }

  this->clipboard().Clear(CLIPBOARD_TYPE_COPY_PASTE);

  EXPECT_TRUE(this->GetAvailableTypes(CLIPBOARD_TYPE_COPY_PASTE).empty());
  EXPECT_FALSE(this->clipboard().IsFormatAvailable(
      Clipboard::GetPlainTextWFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_FALSE(this->clipboard().IsFormatAvailable(
      Clipboard::GetPlainTextFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
}

TYPED_TEST(ClipboardTest, TextTest) {
  base::string16 text(ASCIIToUTF16("This is a base::string16!#$")), text_result;
  std::string ascii_text;

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteText(text);
  }

  EXPECT_THAT(this->GetAvailableTypes(CLIPBOARD_TYPE_COPY_PASTE),
              Contains(ASCIIToUTF16(Clipboard::kMimeTypeText)));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetPlainTextWFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetPlainTextFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  this->clipboard().ReadText(CLIPBOARD_TYPE_COPY_PASTE, &text_result);

  EXPECT_EQ(text, text_result);
  this->clipboard().ReadAsciiText(CLIPBOARD_TYPE_COPY_PASTE, &ascii_text);
  EXPECT_EQ(UTF16ToUTF8(text), ascii_text);
}

TYPED_TEST(ClipboardTest, HTMLTest) {
  base::string16 markup(ASCIIToUTF16("<string>Hi!</string>")), markup_result;
  base::string16 plain(ASCIIToUTF16("Hi!")), plain_result;
  std::string url("http://www.example.com/"), url_result;

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteText(plain);
    clipboard_writer.WriteHTML(markup, url);
  }

  EXPECT_THAT(this->GetAvailableTypes(CLIPBOARD_TYPE_COPY_PASTE),
              Contains(ASCIIToUTF16(Clipboard::kMimeTypeHTML)));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetHtmlFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  uint32_t fragment_start;
  uint32_t fragment_end;
  this->clipboard().ReadHTML(CLIPBOARD_TYPE_COPY_PASTE, &markup_result,
                             &url_result, &fragment_start, &fragment_end);
  EXPECT_LE(markup.size(), fragment_end - fragment_start);
  EXPECT_EQ(markup,
            markup_result.substr(fragment_end - markup.size(), markup.size()));
#if defined(OS_WIN)
  // TODO(playmobil): It's not clear that non windows clipboards need to support
  // this.
  EXPECT_EQ(url, url_result);
#endif  // defined(OS_WIN)
}

TYPED_TEST(ClipboardTest, RTFTest) {
  std::string rtf =
      "{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\n"
      "This is some {\\b bold} text.\\par\n"
      "}";

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteRTF(rtf);
  }

  EXPECT_THAT(this->GetAvailableTypes(CLIPBOARD_TYPE_COPY_PASTE),
              Contains(ASCIIToUTF16(Clipboard::kMimeTypeRTF)));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(Clipboard::GetRtfFormatType(),
                                                  CLIPBOARD_TYPE_COPY_PASTE));
  std::string result;
  this->clipboard().ReadRTF(CLIPBOARD_TYPE_COPY_PASTE, &result);
  EXPECT_EQ(rtf, result);
}

// TODO(dnicoara) Enable test once Ozone implements clipboard support:
// crbug.com/361707
#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && !defined(USE_OZONE)
TYPED_TEST(ClipboardTest, MultipleBufferTest) {
  base::string16 text(ASCIIToUTF16("Standard")), text_result;
  base::string16 markup(ASCIIToUTF16("<string>Selection</string>"));
  std::string url("http://www.example.com/"), url_result;

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteText(text);
  }

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_SELECTION);
    clipboard_writer.WriteHTML(markup, url);
  }

  EXPECT_THAT(this->GetAvailableTypes(CLIPBOARD_TYPE_COPY_PASTE),
              Contains(ASCIIToUTF16(Clipboard::kMimeTypeText)));
  EXPECT_THAT(this->GetAvailableTypes(CLIPBOARD_TYPE_SELECTION),
              Contains(ASCIIToUTF16(Clipboard::kMimeTypeHTML)));

  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetPlainTextFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_FALSE(this->clipboard().IsFormatAvailable(
      Clipboard::GetPlainTextFormatType(), CLIPBOARD_TYPE_SELECTION));

  EXPECT_FALSE(this->clipboard().IsFormatAvailable(
      Clipboard::GetHtmlFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetHtmlFormatType(), CLIPBOARD_TYPE_SELECTION));

  this->clipboard().ReadText(CLIPBOARD_TYPE_COPY_PASTE, &text_result);
  EXPECT_EQ(text, text_result);

  base::string16 markup_result;
  uint32_t fragment_start;
  uint32_t fragment_end;
  this->clipboard().ReadHTML(CLIPBOARD_TYPE_SELECTION, &markup_result,
                             &url_result, &fragment_start, &fragment_end);
  EXPECT_LE(markup.size(), fragment_end - fragment_start);
  EXPECT_EQ(markup,
            markup_result.substr(fragment_end - markup.size(), markup.size()));
}
#endif

TYPED_TEST(ClipboardTest, TrickyHTMLTest) {
  base::string16 markup(ASCIIToUTF16("<em>Bye!<!--EndFragment --></em>")),
      markup_result;
  std::string url, url_result;
  base::string16 plain(ASCIIToUTF16("Bye!")), plain_result;

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteText(plain);
    clipboard_writer.WriteHTML(markup, url);
  }

  EXPECT_THAT(this->GetAvailableTypes(CLIPBOARD_TYPE_COPY_PASTE),
              Contains(ASCIIToUTF16(Clipboard::kMimeTypeHTML)));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetHtmlFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  uint32_t fragment_start;
  uint32_t fragment_end;
  this->clipboard().ReadHTML(CLIPBOARD_TYPE_COPY_PASTE, &markup_result,
                             &url_result, &fragment_start, &fragment_end);
  EXPECT_LE(markup.size(), fragment_end - fragment_start);
  EXPECT_EQ(markup,
            markup_result.substr(fragment_end - markup.size(), markup.size()));
#if defined(OS_WIN)
  // TODO(playmobil): It's not clear that non windows clipboards need to support
  // this.
  EXPECT_EQ(url, url_result);
#endif  // defined(OS_WIN)
}

// Some platforms store HTML as UTF-8 internally. Make sure fragment indices are
// adjusted appropriately when converting back to UTF-16.
TYPED_TEST(ClipboardTest, UnicodeHTMLTest) {
  base::string16 markup(UTF8ToUTF16("<div>A \xc3\xb8 \xe6\xb0\xb4</div>")),
      markup_result;
  std::string url, url_result;

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteHTML(markup, url);
  }

  EXPECT_THAT(this->GetAvailableTypes(CLIPBOARD_TYPE_COPY_PASTE),
              Contains(ASCIIToUTF16(Clipboard::kMimeTypeHTML)));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetHtmlFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  uint32_t fragment_start;
  uint32_t fragment_end;
  this->clipboard().ReadHTML(CLIPBOARD_TYPE_COPY_PASTE, &markup_result,
                             &url_result, &fragment_start, &fragment_end);
  EXPECT_LE(markup.size(), fragment_end - fragment_start);
  EXPECT_EQ(markup,
            markup_result.substr(fragment_end - markup.size(), markup.size()));
#if defined(OS_WIN)
  EXPECT_EQ(url, url_result);
#endif
}

// TODO(estade): Port the following test (decide what target we use for urls)
#if !defined(OS_POSIX) || defined(OS_MACOSX)
TYPED_TEST(ClipboardTest, BookmarkTest) {
  base::string16 title(ASCIIToUTF16("The Example Company")), title_result;
  std::string url("http://www.example.com/"), url_result;

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteBookmark(title, url);
  }

  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetUrlWFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  this->clipboard().ReadBookmark(&title_result, &url_result);
  EXPECT_EQ(title, title_result);
  EXPECT_EQ(url, url_result);
}
#endif  // !defined(OS_POSIX) || defined(OS_MACOSX)

TYPED_TEST(ClipboardTest, MultiFormatTest) {
  base::string16 text(ASCIIToUTF16("Hi!")), text_result;
  base::string16 markup(ASCIIToUTF16("<strong>Hi!</string>")), markup_result;
  std::string url("http://www.example.com/"), url_result;
  std::string ascii_text;

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteHTML(markup, url);
    clipboard_writer.WriteText(text);
  }

  EXPECT_THAT(this->GetAvailableTypes(CLIPBOARD_TYPE_COPY_PASTE),
              Contains(ASCIIToUTF16(Clipboard::kMimeTypeHTML)));
  EXPECT_THAT(this->GetAvailableTypes(CLIPBOARD_TYPE_COPY_PASTE),
              Contains(ASCIIToUTF16(Clipboard::kMimeTypeText)));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetHtmlFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetPlainTextWFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetPlainTextFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  uint32_t fragment_start;
  uint32_t fragment_end;
  this->clipboard().ReadHTML(CLIPBOARD_TYPE_COPY_PASTE, &markup_result,
                             &url_result, &fragment_start, &fragment_end);
  EXPECT_LE(markup.size(), fragment_end - fragment_start);
  EXPECT_EQ(markup,
            markup_result.substr(fragment_end - markup.size(), markup.size()));
#if defined(OS_WIN)
  // TODO(playmobil): It's not clear that non windows clipboards need to support
  // this.
  EXPECT_EQ(url, url_result);
#endif  // defined(OS_WIN)
  this->clipboard().ReadText(CLIPBOARD_TYPE_COPY_PASTE, &text_result);
  EXPECT_EQ(text, text_result);
  this->clipboard().ReadAsciiText(CLIPBOARD_TYPE_COPY_PASTE, &ascii_text);
  EXPECT_EQ(UTF16ToUTF8(text), ascii_text);
}

TYPED_TEST(ClipboardTest, URLTest) {
  base::string16 url(ASCIIToUTF16("http://www.google.com/"));

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteText(url);
  }

  EXPECT_THAT(this->GetAvailableTypes(CLIPBOARD_TYPE_COPY_PASTE),
              Contains(ASCIIToUTF16(Clipboard::kMimeTypeText)));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetPlainTextWFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetPlainTextFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  base::string16 text_result;
  this->clipboard().ReadText(CLIPBOARD_TYPE_COPY_PASTE, &text_result);

  EXPECT_EQ(text_result, url);

  std::string ascii_text;
  this->clipboard().ReadAsciiText(CLIPBOARD_TYPE_COPY_PASTE, &ascii_text);
  EXPECT_EQ(UTF16ToUTF8(url), ascii_text);

#if defined(OS_POSIX) && !defined(OS_MACOSX) && !defined(OS_ANDROID) && \
    !defined(OS_CHROMEOS)
  if (!this->IsMusTest()) {
    ascii_text.clear();
    this->clipboard().ReadAsciiText(CLIPBOARD_TYPE_SELECTION, &ascii_text);
    EXPECT_EQ(UTF16ToUTF8(url), ascii_text);
  }
#endif
}

static void TestBitmapWrite(Clipboard* clipboard,
                            const gfx::Size& size,
                            const uint32_t* bitmap_data) {
  {
    ScopedClipboardWriter scw(CLIPBOARD_TYPE_COPY_PASTE);
    SkBitmap bitmap;
    ASSERT_TRUE(bitmap.setInfo(
        SkImageInfo::MakeN32Premul(size.width(), size.height())));
    bitmap.setPixels(
        const_cast<void*>(reinterpret_cast<const void*>(bitmap_data)));
    scw.WriteImage(bitmap);
  }

  EXPECT_TRUE(clipboard->IsFormatAvailable(Clipboard::GetBitmapFormatType(),
                                           CLIPBOARD_TYPE_COPY_PASTE));
  const SkBitmap& image = clipboard->ReadImage(CLIPBOARD_TYPE_COPY_PASTE);
  EXPECT_EQ(size, gfx::Size(image.width(), image.height()));
  for (int j = 0; j < image.height(); ++j) {
    const uint32_t* row_address = image.getAddr32(0, j);
    for (int i = 0; i < image.width(); ++i) {
      int offset = i + j * image.width();
      EXPECT_EQ(bitmap_data[offset], row_address[i]) << "i = " << i
                                                     << ", j = " << j;
    }
  }
}

TYPED_TEST(ClipboardTest, SharedBitmapTest) {
  const uint32_t fake_bitmap_1[] = {
      0x46061626, 0xf69f5988, 0x793f2937, 0xfa55b986,
      0x78772152, 0x87692a30, 0x36322a25, 0x4320401b,
      0x91848c21, 0xc3177b3c, 0x6946155c, 0x64171952,
  };
  {
    SCOPED_TRACE("first bitmap");
    TestBitmapWrite(&this->clipboard(), gfx::Size(4, 3), fake_bitmap_1);
  }

  const uint32_t fake_bitmap_2[] = {
      0x46061626, 0xf69f5988,
      0x793f2937, 0xfa55b986,
      0x78772152, 0x87692a30,
      0x36322a25, 0x4320401b,
      0x91848c21, 0xc3177b3c,
      0x6946155c, 0x64171952,
      0xa6910313, 0x8302323e,
  };
  {
    SCOPED_TRACE("second bitmap");
    TestBitmapWrite(&this->clipboard(), gfx::Size(2, 7), fake_bitmap_2);
  }
}

TYPED_TEST(ClipboardTest, DataTest) {
  const ui::Clipboard::FormatType kFormat =
      ui::Clipboard::GetFormatType("chromium/x-test-format");
  std::string payload("test string");
  base::Pickle write_pickle;
  write_pickle.WriteString(payload);

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WritePickledData(write_pickle, kFormat);
  }

  ASSERT_TRUE(
      this->clipboard().IsFormatAvailable(kFormat, CLIPBOARD_TYPE_COPY_PASTE));
  std::string output;
  this->clipboard().ReadData(kFormat, &output);
  ASSERT_FALSE(output.empty());

  base::Pickle read_pickle(output.data(), static_cast<int>(output.size()));
  base::PickleIterator iter(read_pickle);
  std::string unpickled_string;
  ASSERT_TRUE(iter.ReadString(&unpickled_string));
  EXPECT_EQ(payload, unpickled_string);
}

TYPED_TEST(ClipboardTest, MultipleDataTest) {
  const ui::Clipboard::FormatType kFormat1 =
      ui::Clipboard::GetFormatType("chromium/x-test-format1");
  std::string payload1("test string1");
  base::Pickle write_pickle1;
  write_pickle1.WriteString(payload1);

  const ui::Clipboard::FormatType kFormat2 =
      ui::Clipboard::GetFormatType("chromium/x-test-format2");
  std::string payload2("test string2");
  base::Pickle write_pickle2;
  write_pickle2.WriteString(payload2);

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WritePickledData(write_pickle1, kFormat1);
    // overwrite the previous pickle for fun
    clipboard_writer.WritePickledData(write_pickle2, kFormat2);
  }

  ASSERT_TRUE(
      this->clipboard().IsFormatAvailable(kFormat2, CLIPBOARD_TYPE_COPY_PASTE));

  // Check string 2.
  std::string output2;
  this->clipboard().ReadData(kFormat2, &output2);
  ASSERT_FALSE(output2.empty());

  base::Pickle read_pickle2(output2.data(), static_cast<int>(output2.size()));
  base::PickleIterator iter2(read_pickle2);
  std::string unpickled_string2;
  ASSERT_TRUE(iter2.ReadString(&unpickled_string2));
  EXPECT_EQ(payload2, unpickled_string2);

  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WritePickledData(write_pickle2, kFormat2);
    // overwrite the previous pickle for fun
    clipboard_writer.WritePickledData(write_pickle1, kFormat1);
  }

  ASSERT_TRUE(
      this->clipboard().IsFormatAvailable(kFormat1, CLIPBOARD_TYPE_COPY_PASTE));

  // Check string 1.
  std::string output1;
  this->clipboard().ReadData(kFormat1, &output1);
  ASSERT_FALSE(output1.empty());

  base::Pickle read_pickle1(output1.data(), static_cast<int>(output1.size()));
  base::PickleIterator iter1(read_pickle1);
  std::string unpickled_string1;
  ASSERT_TRUE(iter1.ReadString(&unpickled_string1));
  EXPECT_EQ(payload1, unpickled_string1);
}

#if !defined(OS_MACOSX) && !defined(OS_ANDROID)
TYPED_TEST(ClipboardTest, HyperlinkTest) {
  const std::string kTitle("The <Example> Company's \"home page\"");
  const std::string kUrl("http://www.example.com?x=3&lt=3#\"'<>");
  const base::string16 kExpectedHtml(UTF8ToUTF16(
      "<a href=\"http://www.example.com?x=3&amp;lt=3#&quot;&#39;&lt;&gt;\">"
      "The &lt;Example&gt; Company&#39;s &quot;home page&quot;</a>"));

  std::string url_result;
  base::string16 html_result;
  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteHyperlink(ASCIIToUTF16(kTitle), kUrl);
  }

  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetHtmlFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
  uint32_t fragment_start;
  uint32_t fragment_end;
  this->clipboard().ReadHTML(CLIPBOARD_TYPE_COPY_PASTE, &html_result,
                             &url_result, &fragment_start, &fragment_end);
  EXPECT_EQ(kExpectedHtml,
            html_result.substr(fragment_end - kExpectedHtml.size(),
                               kExpectedHtml.size()));
}
#endif

TYPED_TEST(ClipboardTest, WebSmartPasteTest) {
  {
    ScopedClipboardWriter clipboard_writer(CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteWebSmartPaste();
  }

  EXPECT_TRUE(this->clipboard().IsFormatAvailable(
      Clipboard::GetWebKitSmartPasteFormatType(), CLIPBOARD_TYPE_COPY_PASTE));
}

#if defined(OS_WIN)  // Windows only tests.
void HtmlTestHelper(const std::string& cf_html,
                    const std::string& expected_html) {
  std::string html;
  ClipboardUtil::CFHtmlToHtml(cf_html, &html, NULL);
  EXPECT_EQ(html, expected_html);
}

TYPED_TEST(ClipboardTest, HtmlTest) {
  // Test converting from CF_HTML format data with <!--StartFragment--> and
  // <!--EndFragment--> comments, like from MS Word.
  HtmlTestHelper(
      "Version:1.0\r\n"
      "StartHTML:0000000105\r\n"
      "EndHTML:0000000199\r\n"
      "StartFragment:0000000123\r\n"
      "EndFragment:0000000161\r\n"
      "\r\n"
      "<html>\r\n"
      "<body>\r\n"
      "<!--StartFragment-->\r\n"
      "\r\n"
      "<p>Foo</p>\r\n"
      "\r\n"
      "<!--EndFragment-->\r\n"
      "</body>\r\n"
      "</html>\r\n\r\n",
      "<p>Foo</p>");

  // Test converting from CF_HTML format data without <!--StartFragment--> and
  // <!--EndFragment--> comments, like from OpenOffice Writer.
  HtmlTestHelper(
      "Version:1.0\r\n"
      "StartHTML:0000000105\r\n"
      "EndHTML:0000000151\r\n"
      "StartFragment:0000000121\r\n"
      "EndFragment:0000000131\r\n"
      "<html>\r\n"
      "<body>\r\n"
      "<p>Foo</p>\r\n"
      "</body>\r\n"
      "</html>\r\n\r\n",
      "<p>Foo</p>");
}
#endif  // defined(OS_WIN)

// Test writing all formats we have simultaneously.
TYPED_TEST(ClipboardTest, WriteEverything) {
  {
    ScopedClipboardWriter writer(CLIPBOARD_TYPE_COPY_PASTE);
    writer.WriteText(UTF8ToUTF16("foo"));
    writer.WriteHTML(UTF8ToUTF16("foo"), "bar");
    writer.WriteBookmark(UTF8ToUTF16("foo"), "bar");
    writer.WriteHyperlink(ASCIIToUTF16("foo"), "bar");
    writer.WriteWebSmartPaste();
    // Left out: WriteFile, WriteFiles, WriteBitmapFromPixels, WritePickledData.
  }

  // Passes if we don't crash.
}

// TODO(dcheng): Fix this test for Android. It's rather involved, since the
// clipboard change listener is posted to the Java message loop, and spinning
// that loop from C++ to trigger the callback in the test requires a non-trivial
// amount of additional work.
#if !defined(OS_ANDROID)
// Simple test that the sequence number appears to change when the clipboard is
// written to.
// TODO(dcheng): Add a version to test CLIPBOARD_TYPE_SELECTION.
TYPED_TEST(ClipboardTest, GetSequenceNumber) {
  const uint64_t first_sequence_number =
      this->clipboard().GetSequenceNumber(CLIPBOARD_TYPE_COPY_PASTE);

  {
    ScopedClipboardWriter writer(CLIPBOARD_TYPE_COPY_PASTE);
    writer.WriteText(UTF8ToUTF16("World"));
  }

  // On some platforms, the sequence number is updated by a UI callback so pump
  // the message loop to make sure we get the notification.
  base::RunLoop().RunUntilIdle();

  const uint64_t second_sequence_number =
      this->clipboard().GetSequenceNumber(CLIPBOARD_TYPE_COPY_PASTE);

  EXPECT_NE(first_sequence_number, second_sequence_number);
}
#endif

// Test that writing empty parameters doesn't try to dereference an empty data
// vector. Not crashing = passing.
TYPED_TEST(ClipboardTest, WriteTextEmptyParams) {
  ScopedClipboardWriter scw(CLIPBOARD_TYPE_COPY_PASTE);
  scw.WriteText(base::string16());
}

TYPED_TEST(ClipboardTest, WriteHTMLEmptyParams) {
  ScopedClipboardWriter scw(CLIPBOARD_TYPE_COPY_PASTE);
  scw.WriteHTML(base::string16(), std::string());
}

TYPED_TEST(ClipboardTest, WriteRTFEmptyParams) {
  ScopedClipboardWriter scw(CLIPBOARD_TYPE_COPY_PASTE);
  scw.WriteRTF(std::string());
}

TYPED_TEST(ClipboardTest, WriteBookmarkEmptyParams) {
  ScopedClipboardWriter scw(CLIPBOARD_TYPE_COPY_PASTE);
  scw.WriteBookmark(base::string16(), std::string());
}

TYPED_TEST(ClipboardTest, WriteHyperlinkEmptyParams) {
  ScopedClipboardWriter scw(CLIPBOARD_TYPE_COPY_PASTE);
  scw.WriteHyperlink(base::string16(), std::string());
}

TYPED_TEST(ClipboardTest, WritePickledData) {
  ScopedClipboardWriter scw(CLIPBOARD_TYPE_COPY_PASTE);
  scw.WritePickledData(base::Pickle(), Clipboard::GetPlainTextFormatType());
}

TYPED_TEST(ClipboardTest, WriteImageEmptyParams) {
  ScopedClipboardWriter scw(CLIPBOARD_TYPE_COPY_PASTE);
  scw.WriteImage(SkBitmap());
}

}  // namespace ui
