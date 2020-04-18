// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/favicon/favicon_loader.h"

#import <UIKit/UIKit.h>

#import "base/mac/foundation_util.h"
#import "base/mac/scoped_block.h"
#include "base/strings/sys_string_conversions.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_callback.h"
#include "ui/gfx/favicon_size.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

struct FaviconLoader::RequestData {
  RequestData() {}
  RequestData(NSString* key, FaviconLoader::ImageCompletionBlock block)
      : key([key copy]), block(block) {}
  ~RequestData() {}

  NSString* key;
  base::mac::ScopedBlock<FaviconLoader::ImageCompletionBlock> block;
};

FaviconLoader::FaviconLoader(favicon::FaviconService* favicon_service)
    : favicon_service_(favicon_service),
      favicon_cache_([[NSCache alloc] init]) {}

FaviconLoader::~FaviconLoader() {}

// TODO(pinkerton): How do we update the favicon if it's changed on the web?
// We can possibly just rely on this class being purged or the app being killed
// to reset it, but then how do we ensure the FaviconService is updated?
UIImage* FaviconLoader::ImageForURL(const GURL& url,
                                    const favicon_base::IconTypeSet& types,
                                    ImageCompletionBlock block) {
  NSString* key = base::SysUTF8ToNSString(url.spec());
  id value = [favicon_cache_ objectForKey:key];
  if (value) {
    // [NSNull null] returns a singleton, so we can use it as a sentinel value
    // and just compare pointers to validate whether the value is the sentinel
    // or a valid UIImage.
    if (value == [NSNull null])
      return [UIImage imageNamed:@"default_favicon"];
    return base::mac::ObjCCastStrict<UIImage>(value);
  }

  // Kick off an async request for the favicon.
  if (favicon_service_) {
    int size = gfx::kFaviconSize * [UIScreen mainScreen].scale;

    std::unique_ptr<RequestData> request_data(new RequestData(key, block));
    favicon_base::FaviconResultsCallback callback =
        base::Bind(&FaviconLoader::OnFaviconAvailable, base::Unretained(this),
                   base::Passed(&request_data));
    favicon_service_->GetFaviconForPageURL(url, types, size, callback,
                                           &cancelable_task_tracker_);
  }

  return [UIImage imageNamed:@"default_favicon"];
}

void FaviconLoader::CancellAllRequests() {
  cancelable_task_tracker_.TryCancelAll();
}

void FaviconLoader::OnFaviconAvailable(
    std::unique_ptr<RequestData> request_data,
    const std::vector<favicon_base::FaviconRawBitmapResult>&
        favicon_bitmap_results) {
  DCHECK(request_data);
  if (favicon_bitmap_results.size() < 1 ||
      !favicon_bitmap_results[0].is_valid()) {
    // Return early if there were no results or if it is invalid, after adding a
    // "no favicon" entry to the cache so that we don't keep trying to fetch a
    // missing favicon over and over.
    [favicon_cache_ setObject:[NSNull null] forKey:[request_data->key copy]];
    return;
  }

  // The favicon code assumes favicons are PNG-encoded.
  NSData* image_data =
      [NSData dataWithBytes:favicon_bitmap_results[0].bitmap_data->front()
                     length:favicon_bitmap_results[0].bitmap_data->size()];
  UIImage* favicon =
      [UIImage imageWithData:image_data scale:[[UIScreen mainScreen] scale]];
  [favicon_cache_ setObject:favicon forKey:[request_data->key copy]];

  // Call the block to tell the caller this is complete.
  if (request_data->block)
    (request_data->block.get())(favicon);
}
