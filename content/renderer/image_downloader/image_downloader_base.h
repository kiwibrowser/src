// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_IMAGE_DOWNLOADER_IMAGE_DOWNLOADER_BASE_H_
#define CONTENT_RENDERER_IMAGE_DOWNLOADER_IMAGE_DOWNLOADER_BASE_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/callback.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_thread_observer.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

namespace content {

class MultiResolutionImageResourceFetcher;
class RenderFrame;

class ImageDownloaderBase : public RenderFrameObserver,
                            public RenderThreadObserver {
 public:
  explicit ImageDownloaderBase(RenderFrame* render_frame);
  ~ImageDownloaderBase() override;

  using DownloadCallback =
      base::OnceCallback<void(int32_t, const std::vector<SkBitmap>&)>;
  // Request to asynchronously download an image. When done, |callback| will be
  // called.
  void DownloadImage(const GURL& url,
                     bool is_favicon,
                     bool bypass_cache,
                     DownloadCallback callback);

 protected:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

 private:
  // Requests to fetch an image. When done, the image downloader is notified by
  // way of DidFetchImage. If the image is a favicon, cookies will not be sent
  // nor accepted during download. If the image has multiple frames, all frames
  // are returned.
  void FetchImage(const GURL& image_url,
                  bool is_favicon,
                  bool bypass_cache,
                  DownloadCallback callback);

  // This callback is triggered when FetchImage completes, either
  // successfully or with a failure. See FetchImage for more
  // details.
  void DidFetchImage(DownloadCallback callback,
                     MultiResolutionImageResourceFetcher* fetcher,
                     const std::vector<SkBitmap>& images);

  typedef std::vector<std::unique_ptr<MultiResolutionImageResourceFetcher>>
      ImageResourceFetcherList;

  // ImageResourceFetchers schedule via FetchImage.
  ImageResourceFetcherList image_fetchers_;

  DISALLOW_COPY_AND_ASSIGN(ImageDownloaderBase);
};

}  // namespace content

#endif  // CONTENT_RENDERER_IMAGE_DOWNLOADER_IMAGE_DOWNLOADER_BASE_H_
