// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/canvas/canvas2d/clip_list.h"

#include "third_party/blink/renderer/platform/graphics/paint/paint_canvas.h"
#include "third_party/blink/renderer/platform/transforms/affine_transform.h"
#include "third_party/skia/include/pathops/SkPathOps.h"

namespace blink {

ClipList::ClipList(const ClipList& other) : clip_list_(other.clip_list_) {}

void ClipList::ClipPath(const SkPath& path,
                        AntiAliasingMode anti_aliasing_mode,
                        const SkMatrix& ctm) {
  ClipOp new_clip;
  new_clip.anti_aliasing_mode_ = anti_aliasing_mode;
  new_clip.path_ = path;
  new_clip.path_.transform(ctm);
  if (clip_list_.IsEmpty()) {
    current_clip_path_ = path;
  } else {
    Op(current_clip_path_, path, SkPathOp::kIntersect_SkPathOp,
       &current_clip_path_);
  }
  clip_list_.push_back(new_clip);
}

void ClipList::Playback(PaintCanvas* canvas) const {
  for (const ClipOp* it = clip_list_.begin(); it < clip_list_.end(); it++) {
    canvas->clipPath(it->path_, SkClipOp::kIntersect,
                     it->anti_aliasing_mode_ == kAntiAliased);
  }
}

const SkPath& ClipList::GetCurrentClipPath() const {
  return current_clip_path_;
}

ClipList::ClipOp::ClipOp() : anti_aliasing_mode_(kAntiAliased) {}

ClipList::ClipOp::ClipOp(const ClipOp& other) = default;

}  // namespace blink
