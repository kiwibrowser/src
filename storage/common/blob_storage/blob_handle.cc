// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/common/blob_storage/blob_handle.h"

namespace storage {

BlobHandle::BlobHandle(blink::mojom::BlobPtr blob) : blob_(std::move(blob)) {
  DCHECK(blob_);
}

blink::mojom::BlobPtr BlobHandle::Clone() const {
  blink::mojom::BlobPtr clone;
  blob_->Clone(MakeRequest(&clone));
  return clone;
}

BlobHandle::~BlobHandle() = default;

}  // namespace storage
