// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_SCOPED_IMAGE_FLAGS_H_
#define CC_PAINT_SCOPED_IMAGE_FLAGS_H_

#include "base/macros.h"
#include "cc/paint/paint_export.h"
#include "cc/paint/paint_op_buffer.h"

namespace cc {
class ImageProvider;

// A helper class to decode images inside the provided |flags| and provide a
// PaintFlags with the decoded images that can directly be used for
// rasterization.
// This class should only be used if |flags| has any discardable images.
class CC_PAINT_EXPORT ScopedImageFlags {
 public:
  // |image_provider| must outlive this class.
  ScopedImageFlags(ImageProvider* image_provider,
                   const PaintFlags& flags,
                   const SkMatrix& ctm);
  ~ScopedImageFlags();

  // The usage of these flags should not extend beyond the lifetime of this
  // object.
  PaintFlags* decoded_flags() {
    return decoded_flags_ ? &decoded_flags_.value() : nullptr;
  }

 private:
  // An ImageProvider that passes decode requests through to the
  // |source_provider| but keeps the decode cached throughtout its lifetime,
  // instead of passing the ref to the caller.
  class DecodeStashingImageProvider : public ImageProvider {
   public:
    // |source_provider| must outlive this class.
    explicit DecodeStashingImageProvider(ImageProvider* source_provider);
    ~DecodeStashingImageProvider() override;

    // ImageProvider implementation.
    ScopedDecodedDrawImage GetDecodedDrawImage(
        const DrawImage& draw_image) override;

   private:
    ImageProvider* source_provider_;
    std::vector<ScopedDecodedDrawImage> decoded_images_;

    DISALLOW_COPY_AND_ASSIGN(DecodeStashingImageProvider);
  };

  void DecodeImageShader(const PaintFlags& flags, const SkMatrix& ctm);

  void DecodeRecordShader(const PaintFlags& flags, const SkMatrix& ctm);

  base::Optional<PaintFlags> decoded_flags_;
  DecodeStashingImageProvider decode_stashing_image_provider_;

  DISALLOW_COPY_AND_ASSIGN(ScopedImageFlags);
};

}  // namespace cc

#endif  // CC_PAINT_SCOPED_IMAGE_FLAGS_H_
