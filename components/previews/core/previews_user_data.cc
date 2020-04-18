// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_user_data.h"

#include "base/memory/ptr_util.h"
#include "net/url_request/url_request.h"

namespace previews {

const void* const kPreviewsUserDataKey = &kPreviewsUserDataKey;

PreviewsUserData::PreviewsUserData(uint64_t page_id) : page_id_(page_id) {}

PreviewsUserData::~PreviewsUserData() {}

std::unique_ptr<PreviewsUserData> PreviewsUserData::DeepCopy() const {
  std::unique_ptr<PreviewsUserData> copy(new PreviewsUserData(page_id_));
  copy->data_savings_inflation_percent_ = data_savings_inflation_percent_;
  copy->cache_control_no_transform_directive_ =
      cache_control_no_transform_directive_;
  copy->SetCommittedPreviewsType(committed_previews_type_);
  return copy;
}

PreviewsUserData* PreviewsUserData::GetData(const net::URLRequest& request) {
  PreviewsUserData* data =
      static_cast<PreviewsUserData*>(request.GetUserData(kPreviewsUserDataKey));
  return data;
}

PreviewsUserData* PreviewsUserData::Create(net::URLRequest* request,
                                           uint64_t page_id) {
  if (!request)
    return nullptr;
  PreviewsUserData* data = GetData(*request);
  if (data)
    return data;
  data = new PreviewsUserData(page_id);
  request->SetUserData(kPreviewsUserDataKey, base::WrapUnique(data));
  return data;
}

void PreviewsUserData::SetCommittedPreviewsType(
    previews::PreviewsType previews_type) {
  DCHECK(committed_previews_type_ == PreviewsType::NONE);
  committed_previews_type_ = previews_type;
}

}  // namespace previews
