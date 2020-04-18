// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/dom_storage/local_storage_area.h"

#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "third_party/blink/public/platform/web_url.h"

using blink::WebString;
using blink::WebURL;

namespace content {

LocalStorageArea::LocalStorageArea(
    scoped_refptr<LocalStorageCachedArea> cached_area)
    : cached_area_(std::move(cached_area)),
      id_(base::NumberToString(base::RandUint64())) {
  cached_area_->AreaCreated(this);
}

LocalStorageArea::~LocalStorageArea() {
  cached_area_->AreaDestroyed(this);
}

unsigned LocalStorageArea::length() {
  return cached_area_->GetLength();
}

WebString LocalStorageArea::Key(unsigned index) {
  return WebString::FromUTF16(cached_area_->GetKey(index));
}

WebString LocalStorageArea::GetItem(const WebString& key) {
  return WebString::FromUTF16(cached_area_->GetItem(key.Utf16()));
}

void LocalStorageArea::SetItem(const WebString& key,
                               const WebString& value,
                               const WebURL& page_url,
                               WebStorageArea::Result& result) {
  if (!cached_area_->SetItem(key.Utf16(), value.Utf16(), page_url, id_))
    result = kResultBlockedByQuota;
  else
    result = kResultOK;
}

void LocalStorageArea::RemoveItem(const WebString& key,
                                  const WebURL& page_url) {
  cached_area_->RemoveItem(key.Utf16(), page_url, id_);
}

void LocalStorageArea::Clear(const WebURL& page_url) {
  cached_area_->Clear(page_url, id_);
}

}  // namespace content
