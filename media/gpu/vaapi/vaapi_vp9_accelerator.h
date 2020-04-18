// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_VAAPI_VAAPI_VP9_ACCELERATOR_H_
#define MEDIA_GPU_VAAPI_VAAPI_VP9_ACCELERATOR_H_

#include "base/sequence_checker.h"
#include "media/filters/vp9_parser.h"
#include "media/gpu/vp9_decoder.h"

namespace media {

class VP9Picture;
class VaapiVideoDecodeAccelerator;
class VaapiWrapper;

class VaapiVP9Accelerator : public VP9Decoder::VP9Accelerator {
 public:
  VaapiVP9Accelerator(VaapiVideoDecodeAccelerator* vaapi_dec,
                      scoped_refptr<VaapiWrapper> vaapi_wrapper);
  ~VaapiVP9Accelerator() override;

  // VP9Decoder::VP9Accelerator implementation.
  scoped_refptr<VP9Picture> CreateVP9Picture() override;
  bool SubmitDecode(const scoped_refptr<VP9Picture>& pic,
                    const Vp9SegmentationParams& seg,
                    const Vp9LoopFilterParams& lf,
                    const std::vector<scoped_refptr<VP9Picture>>& ref_pictures,
                    const base::Closure& done_cb) override;

  bool OutputPicture(const scoped_refptr<VP9Picture>& pic) override;
  bool IsFrameContextRequired() const override;
  bool GetFrameContext(const scoped_refptr<VP9Picture>& pic,
                       Vp9FrameContext* frame_ctx) override;

 private:
  const scoped_refptr<VaapiWrapper> vaapi_wrapper_;
  VaapiVideoDecodeAccelerator* vaapi_dec_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(VaapiVP9Accelerator);
};

}  // namespace media

#endif  // MEDIA_GPU_VAAPI_VAAPI_VP9_ACCELERATOR_H_
