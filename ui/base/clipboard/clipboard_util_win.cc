// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/clipboard/clipboard_util_win.h"

#include <shellapi.h>
#include <shlwapi.h>
#include <wininet.h>  // For INTERNET_MAX_URL_LENGTH.

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_hglobal.h"
#include "net/base/filename_util.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/custom_data_helper.h"
#include "url/gurl.h"

namespace ui {

namespace {

bool HasData(IDataObject* data_object, const Clipboard::FormatType& format) {
  FORMATETC format_etc = format.ToFormatEtc();
  return SUCCEEDED(data_object->QueryGetData(&format_etc));
}

bool GetData(IDataObject* data_object,
             const Clipboard::FormatType& format,
             STGMEDIUM* medium) {
  FORMATETC format_etc = format.ToFormatEtc();
  return SUCCEEDED(data_object->GetData(&format_etc, medium));
}

bool GetUrlFromHDrop(IDataObject* data_object,
                     GURL* url,
                     base::string16* title) {
  DCHECK(data_object && url && title);

  bool success = false;
  STGMEDIUM medium;
  if (!GetData(data_object, Clipboard::GetCFHDropFormatType(), &medium))
    return false;

  {
    base::win::ScopedHGlobal<HDROP> hdrop(medium.hGlobal);

    if (!hdrop.get())
      return false;

    wchar_t filename[MAX_PATH];
    if (DragQueryFileW(hdrop.get(), 0, filename, arraysize(filename))) {
      wchar_t url_buffer[INTERNET_MAX_URL_LENGTH];
      if (0 == _wcsicmp(PathFindExtensionW(filename), L".url") &&
          GetPrivateProfileStringW(L"InternetShortcut",
                                   L"url",
                                   0,
                                   url_buffer,
                                   arraysize(url_buffer),
                                   filename)) {
        *url = GURL(url_buffer);
        PathRemoveExtension(filename);
        title->assign(PathFindFileName(filename));
        success = url->is_valid();
      }
    }
  }

  ReleaseStgMedium(&medium);
  return success;
}

void SplitUrlAndTitle(const base::string16& str,
                      GURL* url,
                      base::string16* title) {
  DCHECK(url && title);
  size_t newline_pos = str.find('\n');
  if (newline_pos != base::string16::npos) {
    *url = GURL(base::string16(str, 0, newline_pos));
    title->assign(str, newline_pos + 1, base::string16::npos);
  } else {
    *url = GURL(str);
    title->assign(str);
  }
}

}  // namespace

bool ClipboardUtil::HasUrl(IDataObject* data_object, bool convert_filenames) {
  DCHECK(data_object);
  return HasData(data_object, Clipboard::GetMozUrlFormatType()) ||
         HasData(data_object, Clipboard::GetUrlWFormatType()) ||
         HasData(data_object, Clipboard::GetUrlFormatType()) ||
         (convert_filenames && HasFilenames(data_object));
}

bool ClipboardUtil::HasFilenames(IDataObject* data_object) {
  DCHECK(data_object);
  return HasData(data_object, Clipboard::GetCFHDropFormatType()) ||
         HasData(data_object, Clipboard::GetFilenameWFormatType()) ||
         HasData(data_object, Clipboard::GetFilenameFormatType());
}

bool ClipboardUtil::HasFileContents(IDataObject* data_object) {
  DCHECK(data_object);
  return HasData(data_object, Clipboard::GetFileContentZeroFormatType());
}

bool ClipboardUtil::HasHtml(IDataObject* data_object) {
  DCHECK(data_object);
  return HasData(data_object, Clipboard::GetHtmlFormatType()) ||
         HasData(data_object, Clipboard::GetTextHtmlFormatType());
}

bool ClipboardUtil::HasPlainText(IDataObject* data_object) {
  DCHECK(data_object);
  return HasData(data_object, Clipboard::GetPlainTextWFormatType()) ||
         HasData(data_object, Clipboard::GetPlainTextFormatType());
}

bool ClipboardUtil::GetUrl(IDataObject* data_object,
                           GURL* url,
                           base::string16* title,
                           bool convert_filenames) {
  DCHECK(data_object && url && title);
  if (!HasUrl(data_object, convert_filenames))
    return false;

  // Try to extract a URL from |data_object| in a variety of formats.
  STGMEDIUM store;
  if (GetUrlFromHDrop(data_object, url, title))
    return true;

  if (GetData(data_object, Clipboard::GetMozUrlFormatType(), &store) ||
      GetData(data_object, Clipboard::GetUrlWFormatType(), &store)) {
    {
      // Mozilla URL format or unicode URL
      base::win::ScopedHGlobal<wchar_t*> data(store.hGlobal);
      SplitUrlAndTitle(data.get(), url, title);
    }
    ReleaseStgMedium(&store);
    return url->is_valid();
  }

  if (GetData(data_object, Clipboard::GetUrlFormatType(), &store)) {
    {
      // URL using ascii
      base::win::ScopedHGlobal<char*> data(store.hGlobal);
      SplitUrlAndTitle(base::UTF8ToWide(data.get()), url, title);
    }
    ReleaseStgMedium(&store);
    return url->is_valid();
  }

  if (convert_filenames) {
    std::vector<base::string16> filenames;
    if (!GetFilenames(data_object, &filenames))
      return false;
    DCHECK_GT(filenames.size(), 0U);
    *url = net::FilePathToFileURL(base::FilePath(filenames[0]));
    return url->is_valid();
  }

  return false;
}

bool ClipboardUtil::GetFilenames(IDataObject* data_object,
                                 std::vector<base::string16>* filenames) {
  DCHECK(data_object && filenames);
  if (!HasFilenames(data_object))
    return false;

  STGMEDIUM medium;
  if (GetData(data_object, Clipboard::GetCFHDropFormatType(), &medium)) {
    {
      base::win::ScopedHGlobal<HDROP> hdrop(medium.hGlobal);
      if (!hdrop.get())
        return false;

      const int kMaxFilenameLen = 4096;
      const unsigned num_files = DragQueryFileW(hdrop.get(), 0xffffffff, 0, 0);
      for (unsigned int i = 0; i < num_files; ++i) {
        wchar_t filename[kMaxFilenameLen];
        if (!DragQueryFileW(hdrop.get(), i, filename, kMaxFilenameLen))
          continue;
        filenames->push_back(filename);
      }
    }
    ReleaseStgMedium(&medium);
    return true;
  }

  if (GetData(data_object, Clipboard::GetFilenameWFormatType(), &medium)) {
    {
      // filename using unicode
      base::win::ScopedHGlobal<wchar_t*> data(medium.hGlobal);
      if (data.get() && data.get()[0])
        filenames->push_back(data.get());
    }
    ReleaseStgMedium(&medium);
    return true;
  }

  if (GetData(data_object, Clipboard::GetFilenameFormatType(), &medium)) {
    {
      // filename using ascii
      base::win::ScopedHGlobal<char*> data(medium.hGlobal);
      if (data.get() && data.get()[0])
        filenames->push_back(base::SysNativeMBToWide(data.get()));
    }
    ReleaseStgMedium(&medium);
    return true;
  }

  return false;
}

bool ClipboardUtil::GetPlainText(IDataObject* data_object,
                                 base::string16* plain_text) {
  DCHECK(data_object && plain_text);
  if (!HasPlainText(data_object))
    return false;

  STGMEDIUM store;
  if (GetData(data_object, Clipboard::GetPlainTextWFormatType(), &store)) {
    {
      // Unicode text
      base::win::ScopedHGlobal<wchar_t*> data(store.hGlobal);
      plain_text->assign(data.get());
    }
    ReleaseStgMedium(&store);
    return true;
  }

  if (GetData(data_object, Clipboard::GetPlainTextFormatType(), &store)) {
    {
      // ascii text
      base::win::ScopedHGlobal<char*> data(store.hGlobal);
      plain_text->assign(base::UTF8ToWide(data.get()));
    }
    ReleaseStgMedium(&store);
    return true;
  }

  // If a file is dropped on the window, it does not provide either of the
  // plain text formats, so here we try to forcibly get a url.
  GURL url;
  base::string16 title;
  if (GetUrl(data_object, &url, &title, false)) {
    *plain_text = base::UTF8ToUTF16(url.spec());
    return true;
  }
  return false;
}

bool ClipboardUtil::GetHtml(IDataObject* data_object,
                            base::string16* html, std::string* base_url) {
  DCHECK(data_object && html && base_url);

  STGMEDIUM store;
  if (HasData(data_object, Clipboard::GetHtmlFormatType()) &&
      GetData(data_object, Clipboard::GetHtmlFormatType(), &store)) {
    {
      // MS CF html
      base::win::ScopedHGlobal<char*> data(store.hGlobal);

      std::string html_utf8;
      CFHtmlToHtml(std::string(data.get(), data.Size()), &html_utf8, base_url);
      html->assign(base::UTF8ToWide(html_utf8));
    }
    ReleaseStgMedium(&store);
    return true;
  }

  if (!HasData(data_object, Clipboard::GetTextHtmlFormatType()))
    return false;

  if (!GetData(data_object, Clipboard::GetTextHtmlFormatType(), &store))
    return false;

  {
    // text/html
    base::win::ScopedHGlobal<wchar_t*> data(store.hGlobal);
    html->assign(data.get());
  }
  ReleaseStgMedium(&store);
  return true;
}

bool ClipboardUtil::GetFileContents(IDataObject* data_object,
    base::string16* filename, std::string* file_contents) {
  DCHECK(data_object && filename && file_contents);
  if (!HasData(data_object, Clipboard::GetFileContentZeroFormatType()) &&
      !HasData(data_object, Clipboard::GetFileDescriptorFormatType()))
    return false;

  STGMEDIUM content;
  // The call to GetData can be very slow depending on what is in
  // |data_object|.
  if (GetData(
          data_object, Clipboard::GetFileContentZeroFormatType(), &content)) {
    if (TYMED_HGLOBAL == content.tymed) {
      base::win::ScopedHGlobal<char*> data(content.hGlobal);
      file_contents->assign(data.get(), data.Size());
    }
    ReleaseStgMedium(&content);
  }

  STGMEDIUM description;
  if (GetData(data_object,
              Clipboard::GetFileDescriptorFormatType(),
              &description)) {
    {
      base::win::ScopedHGlobal<FILEGROUPDESCRIPTOR*> fgd(description.hGlobal);
      // We expect there to be at least one file in here.
      DCHECK_GE(fgd->cItems, 1u);
      filename->assign(fgd->fgd[0].cFileName);
    }
    ReleaseStgMedium(&description);
  }
  return true;
}

bool ClipboardUtil::GetWebCustomData(
    IDataObject* data_object,
    std::unordered_map<base::string16, base::string16>* custom_data) {
  DCHECK(data_object && custom_data);

  if (!HasData(data_object, Clipboard::GetWebCustomDataFormatType()))
    return false;

  STGMEDIUM store;
  if (GetData(data_object, Clipboard::GetWebCustomDataFormatType(), &store)) {
    {
      base::win::ScopedHGlobal<char*> data(store.hGlobal);
      ReadCustomDataIntoMap(data.get(), data.Size(), custom_data);
    }
    ReleaseStgMedium(&store);
    return true;
  }
  return false;
}


// HtmlToCFHtml and CFHtmlToHtml are based on similar methods in
// WebCore/platform/win/ClipboardUtilitiesWin.cpp.
/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Helper method for converting from text/html to MS CF_HTML.
// Documentation for the CF_HTML format is available at
// http://msdn.microsoft.com/en-us/library/aa767917(VS.85).aspx
std::string ClipboardUtil::HtmlToCFHtml(const std::string& html,
                                        const std::string& base_url) {
  if (html.empty())
    return std::string();

  #define MAX_DIGITS 10
  #define MAKE_NUMBER_FORMAT_1(digits) MAKE_NUMBER_FORMAT_2(digits)
  #define MAKE_NUMBER_FORMAT_2(digits) "%0" #digits "u"
  #define NUMBER_FORMAT MAKE_NUMBER_FORMAT_1(MAX_DIGITS)

  static const char* header = "Version:0.9\r\n"
      "StartHTML:" NUMBER_FORMAT "\r\n"
      "EndHTML:" NUMBER_FORMAT "\r\n"
      "StartFragment:" NUMBER_FORMAT "\r\n"
      "EndFragment:" NUMBER_FORMAT "\r\n";
  static const char* source_url_prefix = "SourceURL:";

  static const char* start_markup =
      "<html>\r\n<body>\r\n<!--StartFragment-->";
  static const char* end_markup =
      "<!--EndFragment-->\r\n</body>\r\n</html>";

  // Calculate offsets
  size_t start_html_offset = strlen(header) - strlen(NUMBER_FORMAT) * 4 +
      MAX_DIGITS * 4;
  if (!base_url.empty()) {
    start_html_offset += strlen(source_url_prefix) +
        base_url.length() + 2;  // Add 2 for \r\n.
  }
  size_t start_fragment_offset = start_html_offset + strlen(start_markup);
  size_t end_fragment_offset = start_fragment_offset + html.length();
  size_t end_html_offset = end_fragment_offset + strlen(end_markup);

  std::string result = base::StringPrintf(header,
                                          start_html_offset,
                                          end_html_offset,
                                          start_fragment_offset,
                                          end_fragment_offset);
  if (!base_url.empty()) {
    result.append(source_url_prefix);
    result.append(base_url);
    result.append("\r\n");
  }
  result.append(start_markup);
  result.append(html);
  result.append(end_markup);

  #undef MAX_DIGITS
  #undef MAKE_NUMBER_FORMAT_1
  #undef MAKE_NUMBER_FORMAT_2
  #undef NUMBER_FORMAT

  return result;
}

// Helper method for converting from MS CF_HTML to text/html.
void ClipboardUtil::CFHtmlToHtml(const std::string& cf_html,
                                 std::string* html,
                                 std::string* base_url) {
  size_t fragment_start = std::string::npos;
  size_t fragment_end = std::string::npos;

  ClipboardUtil::CFHtmlExtractMetadata(
      cf_html, base_url, NULL, &fragment_start, &fragment_end);

  if (html &&
      fragment_start != std::string::npos &&
      fragment_end != std::string::npos) {
    *html = cf_html.substr(fragment_start, fragment_end - fragment_start);
    base::TrimWhitespaceASCII(*html, base::TRIM_ALL, html);
  }
}

void ClipboardUtil::CFHtmlExtractMetadata(const std::string& cf_html,
                                          std::string* base_url,
                                          size_t* html_start,
                                          size_t* fragment_start,
                                          size_t* fragment_end) {
  // Obtain base_url if present.
  if (base_url) {
    static constexpr char kSrcUrlStr[] = "SourceURL:";
    size_t line_start = cf_html.find(kSrcUrlStr);
    if (line_start != std::string::npos) {
      size_t src_end = cf_html.find("\n", line_start);
      size_t src_start = line_start + strlen(kSrcUrlStr);
      if (src_end != std::string::npos && src_start != std::string::npos) {
        *base_url = cf_html.substr(src_start, src_end - src_start);
        base::TrimWhitespaceASCII(*base_url, base::TRIM_ALL, base_url);
      }
    }
  }

  // Find the markup between "<!--StartFragment-->" and "<!--EndFragment-->".
  // If the comments cannot be found, like copying from OpenOffice Writer,
  // we simply fall back to using StartFragment/EndFragment bytecount values
  // to determine the fragment indexes.
  std::string cf_html_lower = base::ToLowerASCII(cf_html);
  size_t markup_start = cf_html_lower.find("<html", 0);
  if (html_start) {
    *html_start = markup_start;
  }
  size_t tag_start = cf_html.find("<!--StartFragment", markup_start);
  if (tag_start == std::string::npos) {
    static constexpr char kStartFragmentStr[] = "StartFragment:";
    size_t start_fragment_start = cf_html.find(kStartFragmentStr);
    if (start_fragment_start != std::string::npos) {
      *fragment_start = static_cast<size_t>(atoi(
          cf_html.c_str() + start_fragment_start + strlen(kStartFragmentStr)));
    }

    static constexpr char kEndFragmentStr[] = "EndFragment:";
    size_t end_fragment_start = cf_html.find(kEndFragmentStr);
    if (end_fragment_start != std::string::npos) {
      *fragment_end = static_cast<size_t>(
          atoi(cf_html.c_str() + end_fragment_start + strlen(kEndFragmentStr)));
    }
  } else {
    *fragment_start = cf_html.find('>', tag_start) + 1;
    size_t tag_end = cf_html.rfind("<!--EndFragment", std::string::npos);
    *fragment_end = cf_html.rfind('<', tag_end);
  }
}

}  // namespace ui
