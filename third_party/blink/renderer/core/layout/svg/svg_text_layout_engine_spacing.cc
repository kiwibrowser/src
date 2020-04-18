/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/layout/svg/svg_text_layout_engine_spacing.h"

#include "third_party/blink/renderer/platform/fonts/font.h"
#include "third_party/blink/renderer/platform/text/character.h"

namespace blink {

SVGTextLayoutEngineSpacing::SVGTextLayoutEngineSpacing(const Font& font,
                                                       float effective_zoom)
    : font_(font), last_character_(0), effective_zoom_(effective_zoom) {
  DCHECK(effective_zoom_);
}

float SVGTextLayoutEngineSpacing::CalculateCSSSpacing(UChar current_character) {
  UChar last_character = last_character_;
  last_character_ = current_character;

  if (!font_.GetFontDescription().LetterSpacing() &&
      !font_.GetFontDescription().WordSpacing())
    return 0;

  float spacing = font_.GetFontDescription().LetterSpacing();
  if (current_character && last_character &&
      font_.GetFontDescription().WordSpacing()) {
    if (Character::TreatAsSpace(current_character) &&
        !Character::TreatAsSpace(last_character))
      spacing += font_.GetFontDescription().WordSpacing();
  }

  if (effective_zoom_ != 1)
    spacing = spacing / effective_zoom_;

  return spacing;
}

}  // namespace blink
