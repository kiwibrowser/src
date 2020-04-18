// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/payments/payment_instrument_icon_fetcher.h"

#include "base/base64.h"
#include "base/bind_helpers.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/storage_partition_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/manifest_icon_downloader.h"
#include "content/public/browser/manifest_icon_selector.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"

namespace content {
namespace {

// TODO(zino): Choose appropriate icon size dynamically on different platforms.
// Here we choose a large ideal icon size to be big enough for all platforms.
// Note that we only scale down for this icon size but not scale up.
// Please see: https://crbug.com/763886
const int kPaymentAppIdealIconSize = 0xFFFF;
const int kPaymentAppMinimumIconSize = 0;

void DownloadBestMatchingIcon(
    WebContents* web_contents,
    const std::vector<blink::Manifest::Icon>& icons,
    PaymentInstrumentIconFetcher::PaymentInstrumentIconFetcherCallback
        callback);

void OnIconFetched(
    WebContents* web_contents,
    const std::vector<blink::Manifest::Icon>& icons,
    PaymentInstrumentIconFetcher::PaymentInstrumentIconFetcherCallback callback,
    const SkBitmap& bitmap) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (bitmap.drawsNothing()) {
    if (icons.empty()) {
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::BindOnce(std::move(callback), std::string()));
    } else {
      // If could not download or decode the chosen image(e.g. not supported,
      // invalid), try it again with remaining icons.
      DownloadBestMatchingIcon(web_contents, icons, std::move(callback));
    }
    return;
  }

  gfx::Image decoded_image = gfx::Image::CreateFrom1xBitmap(bitmap);
  scoped_refptr<base::RefCountedMemory> raw_data = decoded_image.As1xPNGBytes();
  std::string encoded_data;
  base::Base64Encode(
      base::StringPiece(raw_data->front_as<char>(), raw_data->size()),
      &encoded_data);
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(std::move(callback), encoded_data));
}

void DownloadBestMatchingIcon(
    WebContents* web_contents,
    const std::vector<blink::Manifest::Icon>& icons,
    PaymentInstrumentIconFetcher::PaymentInstrumentIconFetcherCallback
        callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  GURL icon_url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kPaymentAppIdealIconSize, kPaymentAppMinimumIconSize,
      blink::Manifest::Icon::IconPurpose::ANY);
  if (web_contents == nullptr || !icon_url.is_valid()) {
    // If the icon url is invalid, it's better to give the information to
    // developers in advance unlike when fetching or decoding fails. We already
    // checked whether they are valid in renderer side. So, if the icon url is
    // invalid, it's something wrong.
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::BindOnce(std::move(callback), std::string()));
    return;
  }

  std::vector<blink::Manifest::Icon> copy_icons;
  for (const auto& icon : icons) {
    if (icon.src != icon_url) {
      copy_icons.emplace_back(icon);
    }
  }

  bool can_download_icon = ManifestIconDownloader::Download(
      web_contents, icon_url, kPaymentAppIdealIconSize,
      kPaymentAppMinimumIconSize,
      base::Bind(&OnIconFetched, web_contents, copy_icons,
                 base::Passed(std::move(callback))));
  DCHECK(can_download_icon);
}

WebContents* GetWebContentsFromProviderHostIds(
    const GURL& scope,
    std::unique_ptr<std::vector<std::pair<int, int>>> provider_hosts) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  for (const auto& host : *provider_hosts) {
    RenderFrameHostImpl* render_frame_host =
        RenderFrameHostImpl::FromID(host.first, host.second);
    if (!render_frame_host)
      continue;

    WebContentsImpl* web_contents = static_cast<WebContentsImpl*>(
        WebContents::FromRenderFrameHost(render_frame_host));
    if (!web_contents || web_contents->IsHidden() ||
        scope.GetOrigin().spec().compare(
            web_contents->GetLastCommittedURL().GetOrigin().spec()) != 0) {
      continue;
    }
    return web_contents;
  }
  return nullptr;
}

void StartOnUI(
    const GURL& scope,
    std::unique_ptr<std::vector<std::pair<int, int>>> provider_hosts,
    const std::vector<blink::Manifest::Icon>& icons,
    PaymentInstrumentIconFetcher::PaymentInstrumentIconFetcherCallback
        callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  WebContents* web_contents =
      GetWebContentsFromProviderHostIds(scope, std::move(provider_hosts));
  DownloadBestMatchingIcon(web_contents, icons, std::move(callback));
}

}  // namespace

// static
void PaymentInstrumentIconFetcher::Start(
    const GURL& scope,
    std::unique_ptr<std::vector<std::pair<int, int>>> provider_hosts,
    const std::vector<blink::Manifest::Icon>& icons,
    PaymentInstrumentIconFetcherCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&StartOnUI, scope, std::move(provider_hosts), icons,
                     std::move(callback)));
}

}  // namespace content
