// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FILE_UPLOAD_CONTROL_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FILE_UPLOAD_CONTROL_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

struct PaintInfo;
class LayoutPoint;
class LayoutFileUploadControl;

class FileUploadControlPainter {
  STACK_ALLOCATED();

 public:
  FileUploadControlPainter(
      const LayoutFileUploadControl& layout_file_upload_control)
      : layout_file_upload_control_(layout_file_upload_control) {}

  void PaintObject(const PaintInfo&, const LayoutPoint&);

 private:
  const LayoutFileUploadControl& layout_file_upload_control_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FILE_UPLOAD_CONTROL_PAINTER_H_
