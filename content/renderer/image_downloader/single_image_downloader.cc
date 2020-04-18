// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/image_downloader/single_image_downloader.h"

#include "base/bind.h"
#include "base/logging.h"

namespace content {

// Static
void SingleImageDownloader::DownloadImage(
    base::WeakPtr<RenderFrame> render_frame,
    const GURL& url,
    const DownloadImageCallback& cb) {
  DCHECK(!cb.is_null());
  if (!render_frame) {
    cb.Run(SkBitmap());
    return;
  }

  std::unique_ptr<ImageDownloaderBase> image_downloader(
      new ImageDownloaderBase(render_frame.get()));
  ImageDownloaderBase* image_downloader_ptr = image_downloader.get();
  image_downloader_ptr->DownloadImage(
      url, false, false,
      base::BindOnce(&SingleImageDownloader::DidDownloadImage,
                     std::move(image_downloader), cb));
}

// Static
void SingleImageDownloader::DidDownloadImage(
    std::unique_ptr<ImageDownloaderBase> image_downloader,
    const DownloadImageCallback& callback,
    int http_status_code,
    const std::vector<SkBitmap>& images) {
  DCHECK(!callback.is_null());
  callback.Run(images.empty() ? SkBitmap() : images[0]);
}

}  // namespace content
