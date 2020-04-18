/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights
 * reserved.
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
 *
 */

#include "third_party/blink/renderer/core/style/shadow_data.h"

#include "third_party/blink/renderer/platform/animation/animation_utilities.h"
#include "third_party/blink/renderer/platform/graphics/color_blend.h"

namespace blink {

bool ShadowData::operator==(const ShadowData& o) const {
  return location_ == o.location_ && blur_ == o.blur_ && spread_ == o.spread_ &&
         style_ == o.style_ && color_ == o.color_;
}

ShadowData ShadowData::Blend(const ShadowData& from,
                             double progress,
                             const Color& current_color) const {
  DCHECK_EQ(Style(), from.Style());
  return ShadowData(blink::Blend(from.Location(), Location(), progress),
                    clampTo(blink::Blend(from.Blur(), Blur(), progress), 0.0f),
                    blink::Blend(from.Spread(), Spread(), progress), Style(),
                    blink::Blend(from.GetColor().Resolve(current_color),
                                 GetColor().Resolve(current_color), progress));
}

ShadowData ShadowData::NeutralValue() {
  return ShadowData(FloatPoint(0, 0), 0, 0, kNormal,
                    StyleColor(Color::kTransparent));
}

}  // namespace blink
