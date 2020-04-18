// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_IMAGE_DOWNLOADER_SINGLE_IMAGE_DOWNLOADER_H_
#define CONTENT_RENDERER_IMAGE_DOWNLOADER_SINGLE_IMAGE_DOWNLOADER_H_

#include "base/memory/weak_ptr.h"
#include "content/renderer/image_downloader/image_downloader_base.h"

namespace content {

// A one time image downloader that will download a single image. When there are
// multiple frames, returns the first one. Returns an empty bitmap if
// downloading fails. This class does not impose size limitation on the image.
class SingleImageDownloader {
 public:
  using DownloadImageCallback = base::Callback<void(const SkBitmap&)>;

  // Called to download the image in given |url|, and run |cb| when done.
  // A new ImageDownloaderBase will be created and used to download the image,
  // and will be destructed when downloading finishes or |render_frame| is
  // destructed.
  static void DownloadImage(base::WeakPtr<RenderFrame> render_frame,
                            const GURL& url,
                            const DownloadImageCallback& cb);

 private:
  // Callback when downloading finishes. |image_downloader| is passed in as a
  // unique_ptr to keep it alive while downloading and destroy it after this
  // callback is called.
  static void DidDownloadImage(
      std::unique_ptr<ImageDownloaderBase> image_downloader,
      const DownloadImageCallback& callback,
      int http_status_code,
      const std::vector<SkBitmap>& images);

  DISALLOW_COPY_AND_ASSIGN(SingleImageDownloader);
};

}  // namespace content

#endif  // CONTENT_RENDERER_IMAGE_DOWNLOADER_SINGLE_IMAGE_DOWNLOADER_H_
