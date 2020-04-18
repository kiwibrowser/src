/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/theme.h"

namespace blink {

LengthBox Theme::ControlBorder(ControlPart part,
                               const FontDescription&,
                               const LengthBox& zoomed_box,
                               float) const {
  switch (part) {
    case kPushButtonPart:
    case kMenulistPart:
    case kSearchFieldPart:
    case kCheckboxPart:
    case kRadioPart:
      return LengthBox(0);
    default:
      return zoomed_box;
  }
}

LengthBox Theme::ControlPadding(ControlPart part,
                                const FontDescription&,
                                const Length& zoomed_box_top,
                                const Length& zoomed_box_right,
                                const Length& zoomed_box_bottom,
                                const Length& zoomed_box_left,
                                float) const {
  switch (part) {
    case kMenulistPart:
    case kMenulistButtonPart:
    case kCheckboxPart:
    case kRadioPart:
      return LengthBox(0);
    default:
      return LengthBox(zoomed_box_top, zoomed_box_right, zoomed_box_bottom,
                       zoomed_box_left);
  }
}

}  // namespace blink
