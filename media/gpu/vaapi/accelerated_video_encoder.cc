// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi/accelerated_video_encoder.h"

#include "media/base/video_frame.h"

namespace media {

AcceleratedVideoEncoder::EncodeJob::EncodeJob(
    scoped_refptr<VideoFrame> input_frame,
    bool keyframe,
    base::OnceClosure execute_cb)
    : input_frame_(input_frame),
      timestamp_(input_frame->timestamp()),
      keyframe_(keyframe),
      execute_callback_(std::move(execute_cb)) {
  DCHECK(!execute_callback_.is_null());
}

AcceleratedVideoEncoder::EncodeJob::~EncodeJob() = default;

VaapiEncodeJob* AcceleratedVideoEncoder::EncodeJob::AsVaapiEncodeJob() {
  CHECK(false);
  return nullptr;
}

void AcceleratedVideoEncoder::EncodeJob::AddSetupCallback(
    base::OnceClosure cb) {
  DCHECK(!cb.is_null());
  setup_callbacks_.push(std::move(cb));
}

void AcceleratedVideoEncoder::EncodeJob::AddReferencePicture(
    scoped_refptr<CodecPicture> ref_pic) {
  DCHECK(ref_pic);
  reference_pictures_.push_back(ref_pic);
}

void AcceleratedVideoEncoder::EncodeJob::Execute() {
  while (!setup_callbacks_.empty()) {
    std::move(setup_callbacks_.front()).Run();
    setup_callbacks_.pop();
  }

  std::move(execute_callback_).Run();
}

}  // namespace media
