// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_truetype_font_list.h"

#include <pango/pango.h>
#include <pango/pangocairo.h>

#include <string>

#include "ppapi/proxy/serialized_structs.h"

namespace content {

void GetFontFamilies_SlowBlocking(std::vector<std::string>* font_families) {
  PangoFontMap* font_map = ::pango_cairo_font_map_get_default();
  PangoFontFamily** families = nullptr;
  int num_families = 0;
  ::pango_font_map_list_families(font_map, &families, &num_families);

  for (int i = 0; i < num_families; ++i)
    font_families->push_back(::pango_font_family_get_name(families[i]));
  g_free(families);
}

void GetFontsInFamily_SlowBlocking(
    const std::string& family,
    std::vector<ppapi::proxy::SerializedTrueTypeFontDesc>* fonts_in_family) {
  PangoFontMap* font_map = ::pango_cairo_font_map_get_default();
  PangoFontFamily** font_families = nullptr;
  int num_families = 0;
  ::pango_font_map_list_families(font_map, &font_families, &num_families);

  for (int i = 0; i < num_families; ++i) {
    PangoFontFamily* font_family = font_families[i];
    if (family.compare(::pango_font_family_get_name(font_family)) == 0) {
      PangoFontFace** font_faces = nullptr;
      int num_faces = 0;
      ::pango_font_family_list_faces(font_family, &font_faces, &num_faces);

      for (int j = 0; j < num_faces; ++j) {
        PangoFontFace* font_face = font_faces[j];
        PangoFontDescription* font_desc = ::pango_font_face_describe(font_face);
        ppapi::proxy::SerializedTrueTypeFontDesc desc;
        desc.family = family;
        if (::pango_font_description_get_style(font_desc) == PANGO_STYLE_ITALIC)
          desc.style = PP_TRUETYPEFONTSTYLE_ITALIC;
        desc.weight = static_cast<PP_TrueTypeFontWeight_Dev>(
            ::pango_font_description_get_weight(font_desc));
        desc.width = static_cast<PP_TrueTypeFontWidth_Dev>(
            ::pango_font_description_get_stretch(font_desc));
        // Character set is not part of Pango font description.
        desc.charset = PP_TRUETYPEFONTCHARSET_DEFAULT;

        fonts_in_family->push_back(desc);
        ::pango_font_description_free(font_desc);
      }
      g_free(font_faces);
    }
  }
  g_free(font_families);
}

}  // namespace content
