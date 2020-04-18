// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/pepper_cursor_setter.h"

#include <stdint.h>

#include "base/logging.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/mouse_cursor.h"
#include "remoting/client/empty_cursor_filter.h"
#include "remoting/proto/control.pb.h"

namespace remoting {

PepperCursorSetter::PepperCursorSetter(const pp::InstanceHandle& instance)
    : instance_(instance), delegate_stub_(nullptr) {
}

PepperCursorSetter::~PepperCursorSetter() {}

void PepperCursorSetter::SetCursorShape(
    const protocol::CursorShapeInfo& cursor_shape) {
  if (SetInstanceCursor(cursor_shape)) {
    // PPAPI cursor was set successfully, so clear delegate cursor, if any.
    if (delegate_stub_) {
      delegate_stub_->SetCursorShape(EmptyCursorShape());
    }
  } else if (delegate_stub_) {
    // Clear PPAPI cursor and fall-back to rendering cursor via delegate.
    pp::MouseCursor::SetCursor(instance_, PP_MOUSECURSOR_TYPE_NONE);
    delegate_stub_->SetCursorShape(cursor_shape);
  } else {
    // TODO(wez): Fall-back to cropping & re-trying the cursor iff there is no
    // delegate and the cursor is >32x32?
    DLOG(FATAL) << "Failed to set PPAPI cursor, and no delegate provided.";
  }
}

bool PepperCursorSetter::SetInstanceCursor(
    const protocol::CursorShapeInfo& cursor_shape) {
  // If the cursor is empty then tell PPAPI there is no cursor.
  if (IsCursorShapeEmpty(cursor_shape)) {
    pp::MouseCursor::SetCursor(instance_, PP_MOUSECURSOR_TYPE_NONE);
    return true;
  }

  // pp::MouseCursor requires image to be in the native format.
  if (pp::ImageData::GetNativeImageDataFormat() !=
      PP_IMAGEDATAFORMAT_BGRA_PREMUL) {
    LOG(WARNING) << "Unable to set cursor shape - native image format is not"
                    " premultiplied BGRA";
    return false;
  }

  // Create a new ImageData to pass to SetCursor().
  pp::Size size(cursor_shape.width(), cursor_shape.height());
  pp::Point hotspot(cursor_shape.hotspot_x(), cursor_shape.hotspot_y());
  pp::ImageData image(instance_, PP_IMAGEDATAFORMAT_BGRA_PREMUL, size, false);
  if (image.is_null()) {
    LOG(WARNING) << "Unable to create cursor image";
    return false;
  }

  // Fill the pixel data and pass the cursor to PPAPI to set.
  const int kBytesPerPixel = sizeof(uint32_t);
  const uint32_t* src_row_data = reinterpret_cast<const uint32_t*>(
      cursor_shape.data().data());
  const int bytes_per_row = size.width() * kBytesPerPixel;
  uint8_t* dst_row_data = reinterpret_cast<uint8_t*>(image.data());
  for (int row = 0; row < size.height(); row++) {
    memcpy(dst_row_data, src_row_data, bytes_per_row);
    src_row_data += size.width();
    dst_row_data += image.stride();
  }

  return pp::MouseCursor::SetCursor(
      instance_, PP_MOUSECURSOR_TYPE_CUSTOM, image, hotspot);
}

}  // namespace remoting
