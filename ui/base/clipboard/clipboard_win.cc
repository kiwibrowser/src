// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Many of these functions are based on those found in
// webkit/port/platform/PasteboardWin.cpp

#include "ui/base/clipboard/clipboard_win.h"

#include <shellapi.h>
#include <shlobj.h>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/numerics/safe_conversions.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_offset_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/message_window.h"
#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "skia/ext/skia_utils_win.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard_util_win.h"
#include "ui/base/clipboard/custom_data_helper.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/size.h"

namespace ui {

namespace {

// A scoper to impersonate the anonymous token and revert when leaving scope
class AnonymousImpersonator {
 public:
  AnonymousImpersonator() {
    must_revert_ = ::ImpersonateAnonymousToken(::GetCurrentThread());
  }

  ~AnonymousImpersonator() {
    if (must_revert_)
      ::RevertToSelf();
  }

 private:
  BOOL must_revert_;
  DISALLOW_COPY_AND_ASSIGN(AnonymousImpersonator);
};

// A scoper to manage acquiring and automatically releasing the clipboard.
class ScopedClipboard {
 public:
  ScopedClipboard() : opened_(false) { }

  ~ScopedClipboard() {
    if (opened_)
      Release();
  }

  bool Acquire(HWND owner) {
    const int kMaxAttemptsToOpenClipboard = 5;

    if (opened_) {
      NOTREACHED();
      return false;
    }

    // Attempt to open the clipboard, which will acquire the Windows clipboard
    // lock.  This may fail if another process currently holds this lock.
    // We're willing to try a few times in the hopes of acquiring it.
    //
    // This turns out to be an issue when using remote desktop because the
    // rdpclip.exe process likes to read what we've written to the clipboard and
    // send it to the RDP client.  If we open and close the clipboard in quick
    // succession, we might be trying to open it while rdpclip.exe has it open,
    // See Bug 815425.
    //
    // In fact, we believe we'll only spin this loop over remote desktop.  In
    // normal situations, the user is initiating clipboard operations and there
    // shouldn't be contention.

    for (int attempts = 0; attempts < kMaxAttemptsToOpenClipboard; ++attempts) {
      // If we didn't manage to open the clipboard, sleep a bit and be hopeful.
      if (attempts != 0)
        ::Sleep(5);

      if (::OpenClipboard(owner)) {
        opened_ = true;
        return true;
      }
    }

    // We failed to acquire the clipboard.
    return false;
  }

  void Release() {
    if (opened_) {
      // Impersonate the anonymous token during the call to CloseClipboard
      // This prevents Windows 8+ capturing the broker's access token which
      // could be accessed by lower-privileges chrome processes leading to
      // a risk of EoP
      AnonymousImpersonator impersonator;
      ::CloseClipboard();
      opened_ = false;
    } else {
      NOTREACHED();
    }
  }

 private:
  bool opened_;
};

bool ClipboardOwnerWndProc(UINT message,
                           WPARAM wparam,
                           LPARAM lparam,
                           LRESULT* result) {
  switch (message) {
  case WM_RENDERFORMAT:
    // This message comes when SetClipboardData was sent a null data handle
    // and now it's come time to put the data on the clipboard.
    // We always set data, so there isn't a need to actually do anything here.
    break;
  case WM_RENDERALLFORMATS:
    // This message comes when SetClipboardData was sent a null data handle
    // and now this application is about to quit, so it must put data on
    // the clipboard before it exits.
    // We always set data, so there isn't a need to actually do anything here.
    break;
  case WM_DRAWCLIPBOARD:
    break;
  case WM_DESTROY:
    break;
  case WM_CHANGECBCHAIN:
    break;
  default:
    return false;
  }

  *result = 0;
  return true;
}

template <typename charT>
HGLOBAL CreateGlobalData(const std::basic_string<charT>& str) {
  HGLOBAL data =
    ::GlobalAlloc(GMEM_MOVEABLE, ((str.size() + 1) * sizeof(charT)));
  if (data) {
    charT* raw_data = static_cast<charT*>(::GlobalLock(data));
    memcpy(raw_data, str.data(), str.size() * sizeof(charT));
    raw_data[str.size()] = '\0';
    ::GlobalUnlock(data);
  }
  return data;
}

bool BitmapHasInvalidPremultipliedColors(const SkPixmap& pixmap) {
  for (int x = 0; x < pixmap.width(); ++x) {
    for (int y = 0; y < pixmap.height(); ++y) {
      uint32_t pixel = *pixmap.addr32(x, y);
      if (SkColorGetR(pixel) > SkColorGetA(pixel) ||
          SkColorGetG(pixel) > SkColorGetA(pixel) ||
          SkColorGetB(pixel) > SkColorGetA(pixel))
        return true;
    }
  }
  return false;
}

void MakeBitmapOpaque(SkPixmap* pixmap) {
  for (int x = 0; x < pixmap->width(); ++x) {
    for (int y = 0; y < pixmap->height(); ++y) {
      *pixmap->writable_addr32(x, y) = SkColorSetA(*pixmap->addr32(x, y), 0xFF);
    }
  }
}

void ParseBookmarkClipboardFormat(const base::string16& bookmark,
                                  base::string16* title,
                                  std::string* url) {
  const base::string16 kDelim = base::ASCIIToUTF16("\r\n");

  const size_t title_end = bookmark.find_first_of(kDelim);
  if (title)
    title->assign(bookmark.substr(0, title_end));

  if (url) {
    const size_t url_start = bookmark.find_first_not_of(kDelim, title_end);
    if (url_start != base::string16::npos) {
      *url =
          base::UTF16ToUTF8(bookmark.substr(url_start, base::string16::npos));
    }
  }
}

void FreeData(unsigned int format, HANDLE data) {
  if (format == CF_BITMAP)
    ::DeleteObject(static_cast<HBITMAP>(data));
  else
    ::GlobalFree(data);
}

template <typename StringType>
void TrimAfterNull(StringType* result) {
  // Text copied to the clipboard may explicitly contain null characters that
  // should be ignored, depending on the application that does the copying.
  constexpr typename StringType::value_type kNull = 0;
  size_t pos = result->find_first_of(kNull);
  if (pos != StringType::npos)
    result->resize(pos);
}

}  // namespace

// Clipboard::FormatType implementation.
Clipboard::FormatType::FormatType() : data_() {}

Clipboard::FormatType::FormatType(UINT native_format) : data_() {
  // There's no good way to actually initialize this in the constructor in
  // C++03.
  data_.cfFormat = static_cast<CLIPFORMAT>(native_format);
  data_.dwAspect = DVASPECT_CONTENT;
  data_.lindex = -1;
  data_.tymed = TYMED_HGLOBAL;
}

Clipboard::FormatType::FormatType(UINT native_format, LONG index) : data_() {
  // There's no good way to actually initialize this in the constructor in
  // C++03.
  data_.cfFormat = static_cast<CLIPFORMAT>(native_format);
  data_.dwAspect = DVASPECT_CONTENT;
  data_.lindex = index;
  data_.tymed = TYMED_HGLOBAL;
}

Clipboard::FormatType::~FormatType() {
}

std::string Clipboard::FormatType::Serialize() const {
  return base::IntToString(data_.cfFormat);
}

// static
Clipboard::FormatType Clipboard::FormatType::Deserialize(
    const std::string& serialization) {
  int clipboard_format = -1;
  if (!base::StringToInt(serialization, &clipboard_format)) {
    NOTREACHED();
    return FormatType();
  }
  return FormatType(clipboard_format);
}

bool Clipboard::FormatType::operator<(const FormatType& other) const {
  return data_.cfFormat < other.data_.cfFormat;
}

bool Clipboard::FormatType::Equals(const FormatType& other) const {
  return data_.cfFormat == other.data_.cfFormat;
}

// Various predefined FormatTypes.
// static
Clipboard::FormatType Clipboard::GetFormatType(
    const std::string& format_string) {
  return FormatType(
      ::RegisterClipboardFormat(base::ASCIIToUTF16(format_string).c_str()));
}

// The following formats can be referenced by ClipboardUtilWin::GetPlainText.
// For reasons (COM), they must be initialized in a thread-safe manner.
// TODO(dcheng): We probably need to make static initialization of "known"
// FormatTypes thread-safe on all platforms.
#define CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(name, ...) \
  struct FormatTypeArgumentForwarder : public FormatType { \
    FormatTypeArgumentForwarder() : FormatType(__VA_ARGS__) { } \
  }; \
  static base::LazyInstance<FormatTypeArgumentForwarder>::Leaky name = \
      LAZY_INSTANCE_INITIALIZER
// static
const Clipboard::FormatType& Clipboard::GetUrlFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
    type, ::RegisterClipboardFormat(CFSTR_INETURLA));
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetUrlWFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type, ::RegisterClipboardFormat(CFSTR_INETURLW));
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetMozUrlFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type, ::RegisterClipboardFormat(L"text/x-moz-url"));
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetPlainTextFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(type, CF_TEXT);
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetPlainTextWFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(type, CF_UNICODETEXT);
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetFilenameFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type, ::RegisterClipboardFormat(CFSTR_FILENAMEA));
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetFilenameWFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type, ::RegisterClipboardFormat(CFSTR_FILENAMEW));
  return type.Get();
}

// MS HTML Format
// static
const Clipboard::FormatType& Clipboard::GetHtmlFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type, ::RegisterClipboardFormat(L"HTML Format"));
  return type.Get();
}

// MS RTF Format
// static
const Clipboard::FormatType& Clipboard::GetRtfFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type, ::RegisterClipboardFormat(L"Rich Text Format"));
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetBitmapFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(type, CF_BITMAP);
  return type.Get();
}

// Firefox text/html
// static
const Clipboard::FormatType& Clipboard::GetTextHtmlFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type, ::RegisterClipboardFormat(L"text/html"));
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetCFHDropFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(type, CF_HDROP);
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetFileDescriptorFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type, ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR));
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetFileContentZeroFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type, ::RegisterClipboardFormat(CFSTR_FILECONTENTS), 0);
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetIDListFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type, ::RegisterClipboardFormat(CFSTR_SHELLIDLIST));
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetWebKitSmartPasteFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type,
      ::RegisterClipboardFormat(L"WebKit Smart Paste Format"));
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetWebCustomDataFormatType() {
  // TODO(dcheng): This name is temporary. See http://crbug.com/106449.
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type,
      ::RegisterClipboardFormat(L"Chromium Web Custom MIME Data Format"));
  return type.Get();
}

// static
const Clipboard::FormatType& Clipboard::GetPepperCustomDataFormatType() {
  CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE(
      type,
      ::RegisterClipboardFormat(L"Chromium Pepper MIME Data Format"));
  return type.Get();
}
#undef CR_STATIC_UI_CLIPBOARD_FORMAT_TYPE

// Clipboard factory method.
// static
Clipboard* Clipboard::Create() {
  return new ClipboardWin;
}

// ClipboardWin implementation.
ClipboardWin::ClipboardWin() {
  if (base::MessageLoopForUI::IsCurrent())
    clipboard_owner_.reset(new base::win::MessageWindow());
}

ClipboardWin::~ClipboardWin() {
}

void ClipboardWin::OnPreShutdown() {}

uint64_t ClipboardWin::GetSequenceNumber(ClipboardType type) const {
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);
  return ::GetClipboardSequenceNumber();
}

bool ClipboardWin::IsFormatAvailable(const Clipboard::FormatType& format,
                                     ClipboardType type) const {
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);
  return ::IsClipboardFormatAvailable(format.ToFormatEtc().cfFormat) != FALSE;
}

void ClipboardWin::Clear(ClipboardType type) {
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  ::EmptyClipboard();
}

void ClipboardWin::ReadAvailableTypes(ClipboardType type,
                                      std::vector<base::string16>* types,
                                      bool* contains_filenames) const {
  if (!types || !contains_filenames) {
    NOTREACHED();
    return;
  }

  types->clear();
  if (::IsClipboardFormatAvailable(
          GetPlainTextFormatType().ToFormatEtc().cfFormat))
    types->push_back(base::UTF8ToUTF16(kMimeTypeText));
  if (::IsClipboardFormatAvailable(GetHtmlFormatType().ToFormatEtc().cfFormat))
    types->push_back(base::UTF8ToUTF16(kMimeTypeHTML));
  if (::IsClipboardFormatAvailable(GetRtfFormatType().ToFormatEtc().cfFormat))
    types->push_back(base::UTF8ToUTF16(kMimeTypeRTF));
  if (::IsClipboardFormatAvailable(CF_DIB))
    types->push_back(base::UTF8ToUTF16(kMimeTypePNG));
  *contains_filenames = false;

  // Acquire the clipboard.
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE hdata =
      ::GetClipboardData(GetWebCustomDataFormatType().ToFormatEtc().cfFormat);
  if (!hdata)
    return;

  ReadCustomDataTypes(::GlobalLock(hdata), ::GlobalSize(hdata), types);
  ::GlobalUnlock(hdata);
}

void ClipboardWin::ReadText(ClipboardType type, base::string16* result) const {
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);
  if (!result) {
    NOTREACHED();
    return;
  }

  result->clear();

  // Acquire the clipboard.
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE data = ::GetClipboardData(CF_UNICODETEXT);
  if (!data)
    return;

  result->assign(static_cast<const base::char16*>(::GlobalLock(data)),
                 ::GlobalSize(data) / sizeof(base::char16));
  ::GlobalUnlock(data);
  TrimAfterNull(result);
}

void ClipboardWin::ReadAsciiText(ClipboardType type,
                                 std::string* result) const {
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);
  if (!result) {
    NOTREACHED();
    return;
  }

  result->clear();

  // Acquire the clipboard.
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE data = ::GetClipboardData(CF_TEXT);
  if (!data)
    return;

  result->assign(static_cast<const char*>(::GlobalLock(data)),
                 ::GlobalSize(data));
  ::GlobalUnlock(data);
  TrimAfterNull(result);
}

void ClipboardWin::ReadHTML(ClipboardType type,
                            base::string16* markup,
                            std::string* src_url,
                            uint32_t* fragment_start,
                            uint32_t* fragment_end) const {
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);

  markup->clear();
  // TODO(dcheng): Remove these checks, I don't think they should be optional.
  DCHECK(src_url);
  if (src_url)
    src_url->clear();
  *fragment_start = 0;
  *fragment_end = 0;

  // Acquire the clipboard.
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE data = ::GetClipboardData(GetHtmlFormatType().ToFormatEtc().cfFormat);
  if (!data)
    return;

  std::string cf_html(static_cast<const char*>(::GlobalLock(data)),
                      ::GlobalSize(data));
  ::GlobalUnlock(data);
  TrimAfterNull(&cf_html);

  size_t html_start = std::string::npos;
  size_t start_index = std::string::npos;
  size_t end_index = std::string::npos;
  ClipboardUtil::CFHtmlExtractMetadata(cf_html, src_url, &html_start,
                                       &start_index, &end_index);

  // This might happen if the contents of the clipboard changed and CF_HTML is
  // no longer available.
  if (start_index == std::string::npos ||
      end_index == std::string::npos ||
      html_start == std::string::npos)
    return;

  if (start_index < html_start || end_index < start_index)
    return;

  std::vector<size_t> offsets;
  offsets.push_back(start_index - html_start);
  offsets.push_back(end_index - html_start);
  markup->assign(base::UTF8ToUTF16AndAdjustOffsets(cf_html.data() + html_start,
                                                   &offsets));
  // Ensure the Fragment points within the string; see https://crbug.com/607181.
  size_t end = std::min(offsets[1], markup->length());
  *fragment_start = base::checked_cast<uint32_t>(std::min(offsets[0], end));
  *fragment_end = base::checked_cast<uint32_t>(end);
}

void ClipboardWin::ReadRTF(ClipboardType type, std::string* result) const {
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);

  ReadData(GetRtfFormatType(), result);
  TrimAfterNull(result);
}

SkBitmap ClipboardWin::ReadImage(ClipboardType type) const {
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);

  // Acquire the clipboard.
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return SkBitmap();

  // We use a DIB rather than a DDB here since ::GetObject() with the
  // HBITMAP returned from ::GetClipboardData(CF_BITMAP) always reports a color
  // depth of 32bpp.
  BITMAPINFO* bitmap = static_cast<BITMAPINFO*>(::GetClipboardData(CF_DIB));
  if (!bitmap)
    return SkBitmap();
  int color_table_length = 0;
  switch (bitmap->bmiHeader.biBitCount) {
    case 1:
    case 4:
    case 8:
      color_table_length = bitmap->bmiHeader.biClrUsed
          ? bitmap->bmiHeader.biClrUsed
          : 1 << bitmap->bmiHeader.biBitCount;
      break;
    case 16:
    case 32:
      if (bitmap->bmiHeader.biCompression == BI_BITFIELDS)
        color_table_length = 3;
      break;
    case 24:
      break;
    default:
      NOTREACHED();
  }
  const void* bitmap_bits = reinterpret_cast<const char*>(bitmap)
      + bitmap->bmiHeader.biSize + color_table_length * sizeof(RGBQUAD);

  void* dst_bits;
  // dst_hbitmap is freed by the release_proc in skia_bitmap (below)
  HBITMAP dst_hbitmap =
      skia::CreateHBitmap(bitmap->bmiHeader.biWidth, bitmap->bmiHeader.biHeight,
                          false, 0, &dst_bits);

  {
    base::win::ScopedCreateDC hdc(CreateCompatibleDC(NULL));
    HBITMAP old_hbitmap =
        static_cast<HBITMAP>(SelectObject(hdc.Get(), dst_hbitmap));
    ::SetDIBitsToDevice(hdc.Get(), 0, 0, bitmap->bmiHeader.biWidth,
                        bitmap->bmiHeader.biHeight, 0, 0, 0,
                        bitmap->bmiHeader.biHeight, bitmap_bits, bitmap,
                        DIB_RGB_COLORS);
    SelectObject(hdc.Get(), old_hbitmap);
  }
  // Windows doesn't really handle alpha channels well in many situations. When
  // the source image is < 32 bpp, we force the bitmap to be opaque. When the
  // source image is 32 bpp, the alpha channel might still contain garbage data.
  // Since Windows uses premultiplied alpha, we scan for instances where
  // (R, G, B) > A. If there are any invalid premultiplied colors in the image,
  // we assume the alpha channel contains garbage and force the bitmap to be
  // opaque as well. Note that this  heuristic will fail on a transparent bitmap
  // containing only black pixels...
  SkPixmap device_pixels(SkImageInfo::MakeN32Premul(bitmap->bmiHeader.biWidth,
                                                    bitmap->bmiHeader.biHeight),
                         dst_bits, bitmap->bmiHeader.biWidth * 4);

  {
    bool has_invalid_alpha_channel = bitmap->bmiHeader.biBitCount < 32 ||
        BitmapHasInvalidPremultipliedColors(device_pixels);
    if (has_invalid_alpha_channel) {
      MakeBitmapOpaque(&device_pixels);
    }
  }

  SkBitmap skia_bitmap;
  skia_bitmap.installPixels(device_pixels.info(), device_pixels.writable_addr(),
                            device_pixels.rowBytes(),
                            [](void* pixels, void* hbitmap) {
                              DeleteObject(static_cast<HBITMAP>(hbitmap));
                            },
                            dst_hbitmap);
  return skia_bitmap;
}

void ClipboardWin::ReadCustomData(ClipboardType clipboard_type,
                                  const base::string16& type,
                                  base::string16* result) const {
  DCHECK_EQ(clipboard_type, CLIPBOARD_TYPE_COPY_PASTE);

  // Acquire the clipboard.
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE hdata =
      ::GetClipboardData(GetWebCustomDataFormatType().ToFormatEtc().cfFormat);
  if (!hdata)
    return;

  ReadCustomDataForType(::GlobalLock(hdata), ::GlobalSize(hdata), type, result);
  ::GlobalUnlock(hdata);
}

void ClipboardWin::ReadBookmark(base::string16* title, std::string* url) const {
  if (title)
    title->clear();

  if (url)
    url->clear();

  // Acquire the clipboard.
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE data = ::GetClipboardData(GetUrlWFormatType().ToFormatEtc().cfFormat);
  if (!data)
    return;

  base::string16 bookmark(static_cast<const base::char16*>(::GlobalLock(data)),
                          ::GlobalSize(data) / sizeof(base::char16));
  ::GlobalUnlock(data);
  TrimAfterNull(&bookmark);

  ParseBookmarkClipboardFormat(bookmark, title, url);
}

void ClipboardWin::ReadData(const FormatType& format,
                            std::string* result) const {
  if (!result) {
    NOTREACHED();
    return;
  }

  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE data = ::GetClipboardData(format.ToFormatEtc().cfFormat);
  if (!data)
    return;

  result->assign(static_cast<const char*>(::GlobalLock(data)),
                 ::GlobalSize(data));
  ::GlobalUnlock(data);
}

void ClipboardWin::WriteObjects(ClipboardType type, const ObjectMap& objects) {
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);

  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  ::EmptyClipboard();

  for (ObjectMap::const_iterator iter = objects.begin(); iter != objects.end();
       ++iter) {
    DispatchObject(static_cast<ObjectType>(iter->first), iter->second);
  }
}

void ClipboardWin::WriteText(const char* text_data, size_t text_len) {
  base::string16 text;
  base::UTF8ToUTF16(text_data, text_len, &text);
  HGLOBAL glob = CreateGlobalData(text);

  WriteToClipboard(CF_UNICODETEXT, glob);
}

void ClipboardWin::WriteHTML(const char* markup_data,
                             size_t markup_len,
                             const char* url_data,
                             size_t url_len) {
  std::string markup(markup_data, markup_len);
  std::string url;

  if (url_len > 0)
    url.assign(url_data, url_len);

  std::string html_fragment = ClipboardUtil::HtmlToCFHtml(markup, url);
  HGLOBAL glob = CreateGlobalData(html_fragment);

  WriteToClipboard(Clipboard::GetHtmlFormatType().ToFormatEtc().cfFormat, glob);
}

void ClipboardWin::WriteRTF(const char* rtf_data, size_t data_len) {
  WriteData(GetRtfFormatType(), rtf_data, data_len);
}

void ClipboardWin::WriteBookmark(const char* title_data,
                                 size_t title_len,
                                 const char* url_data,
                                 size_t url_len) {
  std::string bookmark(title_data, title_len);
  bookmark.append(1, L'\n');
  bookmark.append(url_data, url_len);

  base::string16 wide_bookmark = base::UTF8ToUTF16(bookmark);
  HGLOBAL glob = CreateGlobalData(wide_bookmark);

  WriteToClipboard(GetUrlWFormatType().ToFormatEtc().cfFormat, glob);
}

void ClipboardWin::WriteWebSmartPaste() {
  DCHECK(clipboard_owner_->hwnd() != NULL);
  ::SetClipboardData(GetWebKitSmartPasteFormatType().ToFormatEtc().cfFormat,
                     NULL);
}

void ClipboardWin::WriteBitmap(const SkBitmap& bitmap) {
  HDC dc = ::GetDC(NULL);

  // This doesn't actually cost us a memcpy when the bitmap comes from the
  // renderer as we load it into the bitmap using setPixels which just sets a
  // pointer.  Someone has to memcpy it into GDI, it might as well be us here.

  // TODO(darin): share data in gfx/bitmap_header.cc somehow
  BITMAPINFO bm_info = {};
  bm_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bm_info.bmiHeader.biWidth = bitmap.width();
  bm_info.bmiHeader.biHeight = -bitmap.height();  // sets vertical orientation
  bm_info.bmiHeader.biPlanes = 1;
  bm_info.bmiHeader.biBitCount = 32;
  bm_info.bmiHeader.biCompression = BI_RGB;

  // ::CreateDIBSection allocates memory for us to copy our bitmap into.
  // Unfortunately, we can't write the created bitmap to the clipboard,
  // (see http://msdn2.microsoft.com/en-us/library/ms532292.aspx)
  void* bits;
  HBITMAP source_hbitmap =
      ::CreateDIBSection(dc, &bm_info, DIB_RGB_COLORS, &bits, NULL, 0);

  if (bits && source_hbitmap) {
    // Copy the bitmap out of shared memory and into GDI
    memcpy(bits, bitmap.getPixels(), bitmap.computeByteSize());

    // Now we have an HBITMAP, we can write it to the clipboard
    WriteBitmapFromHandle(source_hbitmap,
                          gfx::Size(bitmap.width(), bitmap.height()));
  }

  ::DeleteObject(source_hbitmap);
  ::ReleaseDC(NULL, dc);
}

void ClipboardWin::WriteData(const FormatType& format,
                             const char* data_data,
                             size_t data_len) {
  HGLOBAL hdata = ::GlobalAlloc(GMEM_MOVEABLE, data_len);
  if (!hdata)
    return;

  char* data = static_cast<char*>(::GlobalLock(hdata));
  memcpy(data, data_data, data_len);
  ::GlobalUnlock(data);
  WriteToClipboard(format.ToFormatEtc().cfFormat, hdata);
}

void ClipboardWin::WriteBitmapFromHandle(HBITMAP source_hbitmap,
                                         const gfx::Size& size) {
  // We would like to just call ::SetClipboardData on the source_hbitmap,
  // but that bitmap might not be of a sort we can write to the clipboard.
  // For this reason, we create a new bitmap, copy the bits over, and then
  // write that to the clipboard.

  HDC dc = ::GetDC(NULL);
  HDC compatible_dc = ::CreateCompatibleDC(NULL);
  HDC source_dc = ::CreateCompatibleDC(NULL);

  // This is the HBITMAP we will eventually write to the clipboard
  HBITMAP hbitmap = ::CreateCompatibleBitmap(dc, size.width(), size.height());
  if (!hbitmap) {
    // Failed to create the bitmap
    ::DeleteDC(compatible_dc);
    ::DeleteDC(source_dc);
    ::ReleaseDC(NULL, dc);
    return;
  }

  HBITMAP old_hbitmap = (HBITMAP)SelectObject(compatible_dc, hbitmap);
  HBITMAP old_source = (HBITMAP)SelectObject(source_dc, source_hbitmap);

  // Now we need to blend it into an HBITMAP we can place on the clipboard
  BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
  ::GdiAlphaBlend(compatible_dc,
                  0,
                  0,
                  size.width(),
                  size.height(),
                  source_dc,
                  0,
                  0,
                  size.width(),
                  size.height(),
                  bf);

  // Clean up all the handles we just opened
  ::SelectObject(compatible_dc, old_hbitmap);
  ::SelectObject(source_dc, old_source);
  ::DeleteObject(old_hbitmap);
  ::DeleteObject(old_source);
  ::DeleteDC(compatible_dc);
  ::DeleteDC(source_dc);
  ::ReleaseDC(NULL, dc);

  WriteToClipboard(CF_BITMAP, hbitmap);
}

void ClipboardWin::WriteToClipboard(unsigned int format, HANDLE handle) {
  DCHECK(clipboard_owner_->hwnd() != NULL);
  if (handle && !::SetClipboardData(format, handle)) {
    DCHECK(ERROR_CLIPBOARD_NOT_OPEN != GetLastError());
    FreeData(format, handle);
  }
}

HWND ClipboardWin::GetClipboardWindow() const {
  if (!clipboard_owner_)
    return NULL;

  if (clipboard_owner_->hwnd() == NULL)
    clipboard_owner_->Create(base::Bind(&ClipboardOwnerWndProc));

  return clipboard_owner_->hwnd();
}

}  // namespace ui
