// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_BLOB_UTILS_H_
#define MEDIA_CAPTURE_VIDEO_BLOB_UTILS_H_

#include "media/capture/mojom/image_capture.mojom.h"

namespace media {

struct VideoCaptureFormat;

// Helper method to create a mojom::Blob out of |buffer|, whose pixel format and
// resolution are taken from |capture_format|. Returns a null BlobPtr in case of
// error.
mojom::BlobPtr Blobify(const uint8_t* buffer,
                       const uint32_t bytesused,
                       const VideoCaptureFormat& capture_format);

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_BLOB_UTILS_H_
