// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_IMAGE_DOWNLOADER_IMAGE_DOWNLOADER_IMPL_H_
#define CONTENT_RENDERER_IMAGE_DOWNLOADER_IMAGE_DOWNLOADER_IMPL_H_

#include "content/common/image_downloader/image_downloader.mojom.h"
#include "content/renderer/image_downloader/image_downloader_base.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

class ImageDownloaderImpl : public mojom::ImageDownloader,
                            public ImageDownloaderBase {
 public:
  ~ImageDownloaderImpl() override;

  static void CreateMojoService(RenderFrame* render_frame,
                                mojom::ImageDownloaderRequest request);

 private:
  ImageDownloaderImpl(RenderFrame* render_frame,
                      mojom::ImageDownloaderRequest request);

  // Override ImageDownloaderBase::OnDestruct().
  void OnDestruct() override;

  // ImageDownloader implementation.
  void DownloadImage(const GURL& url,
                     bool is_favicon,
                     uint32_t max_bitmap_size,
                     bool bypass_cache,
                     DownloadImageCallback callback) override;

  // Called when downloading finishes. All frames in |images| whose size <=
  // |max_image_size| will be returned through |callback|. If all of the frames
  // are larger than |max_image_size|, the smallest frame is resized to
  // |max_image_size| and is the only result. |max_image_size| == 0 is
  // interpreted as no max image size.
  void DidDownloadImage(uint32_t max_bitmap_size,
                        DownloadImageCallback callback,
                        int32_t http_status_code,
                        const std::vector<SkBitmap>& images);

  mojo::Binding<mojom::ImageDownloader> binding_;

  DISALLOW_COPY_AND_ASSIGN(ImageDownloaderImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_IMAGE_DOWNLOADER_IMAGE_DOWNLOADER_IMPL_H_
