/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SCREEN_INFO_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SCREEN_INFO_H_

#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_type.h"
#include "third_party/blink/public/platform/shape_properties.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "ui/gfx/color_space.h"

namespace blink {

struct WebScreenInfo {
  // Device scale factor. Specifies the ratio between physical and logical
  // pixels.
  float device_scale_factor = 1.f;

  // The color space of the output display.
  gfx::ColorSpace color_space;

  // The screen depth in bits per pixel
  int depth = 0;

  // The bits per colour component. This assumes that the colours are balanced
  // equally.
  int depth_per_component = 0;

  // This can be true for black and white printers
  bool is_monochrome = false;

  // This is set from the rcMonitor member of MONITORINFOEX, to whit:
  //   "A RECT structure that specifies the display monitor rectangle,
  //   expressed in virtual-screen coordinates. Note that if the monitor
  //   is not the primary display monitor, some of the rectangle's
  //   coordinates may be negative values."
  WebRect rect;

  // This is set from the rcWork member of MONITORINFOEX, to whit:
  //   "A RECT structure that specifies the work area rectangle of the
  //   display monitor that can be used by applications, expressed in
  //   virtual-screen coordinates. Windows uses this rectangle to
  //   maximize an application on the monitor. The rest of the area in
  //   rcMonitor contains system windows such as the task bar and side
  //   bars. Note that if the monitor is not the primary display monitor,
  //   some of the rectangle's coordinates may be negative values".
  WebRect available_rect;

  // This is the orientation 'type' or 'name', as in landscape-primary or
  // portrait-secondary for examples.
  // See WebScreenOrientationType.h for the full list.
  WebScreenOrientationType orientation_type = kWebScreenOrientationUndefined;

  // This is the orientation angle of the displayed content in degrees.
  // It is the opposite of the physical rotation.
  // TODO(crbug.com/840189): we should use an enum rather than a number here.
  uint16_t orientation_angle = 0;

  // This is the shape of display.
  DisplayShape display_shape = kDisplayShapeRect;

  WebScreenInfo() = default;

  bool operator==(const WebScreenInfo& other) const {
    return this->device_scale_factor == other.device_scale_factor &&
           this->color_space == other.color_space &&
           this->depth == other.depth &&
           this->depth_per_component == other.depth_per_component &&
           this->is_monochrome == other.is_monochrome &&
           this->rect == other.rect &&
           this->available_rect == other.available_rect &&
           this->orientation_type == other.orientation_type &&
           this->orientation_angle == other.orientation_angle &&
           this->display_shape == other.display_shape;
  }

  bool operator!=(const WebScreenInfo& other) const {
    return !this->operator==(other);
  }
};

}  // namespace blink

#endif
