// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_truetype_font_list.h"

#include <windows.h>
#include <string.h>

#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_hdc.h"
#include "ppapi/c/dev/ppb_truetype_font_dev.h"
#include "ppapi/proxy/serialized_structs.h"

namespace content {

namespace {

typedef std::vector<std::string> FontFamilyList;
typedef std::vector<ppapi::proxy::SerializedTrueTypeFontDesc> FontDescList;

static int CALLBACK EnumFontFamiliesProc(ENUMLOGFONTEXW* logical_font,
                                         NEWTEXTMETRICEXW* physical_font,
                                         DWORD font_type,
                                         LPARAM lparam) {
  FontFamilyList* font_families = reinterpret_cast<FontFamilyList*>(lparam);
  if (font_families) {
    const LOGFONTW& lf = logical_font->elfLogFont;
    if (lf.lfFaceName[0] && lf.lfFaceName[0] != '@' &&
        lf.lfOutPrecision == OUT_STROKE_PRECIS) {  // Outline fonts only.
      std::string face_name(base::UTF16ToUTF8(lf.lfFaceName));
      font_families->push_back(face_name);
    }
  }
  return 1;
}

static int CALLBACK EnumFontsInFamilyProc(ENUMLOGFONTEXW* logical_font,
                                          NEWTEXTMETRICEXW* physical_font,
                                          DWORD font_type,
                                          LPARAM lparam) {
  FontDescList* fonts_in_family = reinterpret_cast<FontDescList*>(lparam);
  if (fonts_in_family) {
    const LOGFONTW& lf = logical_font->elfLogFont;
    if (lf.lfFaceName[0] && lf.lfFaceName[0] != '@' &&
        lf.lfOutPrecision == OUT_STROKE_PRECIS) {  // Outline fonts only.
      ppapi::proxy::SerializedTrueTypeFontDesc desc;
      desc.family = base::UTF16ToUTF8(lf.lfFaceName);
      if (lf.lfItalic)
        desc.style = PP_TRUETYPEFONTSTYLE_ITALIC;
      desc.weight = static_cast<PP_TrueTypeFontWeight_Dev>(lf.lfWeight);
      desc.width = PP_TRUETYPEFONTWIDTH_NORMAL;  // TODO(bbudge) support widths.
      desc.charset = static_cast<PP_TrueTypeFontCharset_Dev>(lf.lfCharSet);
      fonts_in_family->push_back(desc);
    }
  }
  return 1;
}

}  // namespace

void GetFontFamilies_SlowBlocking(FontFamilyList* font_families) {
  LOGFONTW logfont;
  memset(&logfont, 0, sizeof(logfont));
  logfont.lfCharSet = DEFAULT_CHARSET;
  base::win::ScopedCreateDC hdc(::CreateCompatibleDC(NULL));
  ::EnumFontFamiliesExW(hdc.Get(),
                        &logfont,
                        (FONTENUMPROCW) & EnumFontFamiliesProc,
                        (LPARAM)font_families,
                        0);
}

void GetFontsInFamily_SlowBlocking(const std::string& family,
                                   FontDescList* fonts_in_family) {
  LOGFONTW logfont;
  memset(&logfont, 0, sizeof(logfont));
  logfont.lfCharSet = DEFAULT_CHARSET;
  base::string16 family16 = base::UTF8ToUTF16(family);
  // Copy the family name, leaving room for a terminating null (already set
  // since we zeroed the whole struct above.)
  family16.copy(logfont.lfFaceName, LF_FACESIZE - 1);
  base::win::ScopedCreateDC hdc(::CreateCompatibleDC(NULL));
  ::EnumFontFamiliesExW(hdc.Get(),
                        &logfont,
                        (FONTENUMPROCW) & EnumFontsInFamilyProc,
                        (LPARAM)fonts_in_family,
                        0);
}

}  // namespace content
