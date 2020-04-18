// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/resource_timing_info.h"

#include <memory>
#include "third_party/blink/renderer/platform/cross_thread_copier.h"

namespace blink {

scoped_refptr<ResourceTimingInfo> ResourceTimingInfo::Adopt(
    std::unique_ptr<CrossThreadResourceTimingInfoData> data) {
  scoped_refptr<ResourceTimingInfo> info = ResourceTimingInfo::Create(
      AtomicString(data->type_), data->initial_time_, data->is_main_resource_);
  info->original_timing_allow_origin_ =
      AtomicString(data->original_timing_allow_origin_);
  info->load_finish_time_ = data->load_finish_time_;
  info->initial_url_ = data->initial_url_.Copy();
  info->final_response_ = ResourceResponse(data->final_response_.get());
  for (auto& response_data : data->redirect_chain_)
    info->redirect_chain_.push_back(ResourceResponse(response_data.get()));
  info->transfer_size_ = data->transfer_size_;
  info->negative_allowed_ = data->negative_allowed_;
  return info;
}

std::unique_ptr<CrossThreadResourceTimingInfoData>
ResourceTimingInfo::CopyData() const {
  std::unique_ptr<CrossThreadResourceTimingInfoData> data =
      std::make_unique<CrossThreadResourceTimingInfoData>();
  data->type_ = type_.GetString().IsolatedCopy();
  data->original_timing_allow_origin_ =
      original_timing_allow_origin_.GetString().IsolatedCopy();
  data->initial_time_ = initial_time_;
  data->load_finish_time_ = load_finish_time_;
  data->initial_url_ = initial_url_.Copy();
  data->final_response_ = final_response_.CopyData();
  for (const auto& response : redirect_chain_)
    data->redirect_chain_.push_back(response.CopyData());
  data->transfer_size_ = transfer_size_;
  data->is_main_resource_ = is_main_resource_;
  data->negative_allowed_ = negative_allowed_;
  return data;
}

void ResourceTimingInfo::AddRedirect(const ResourceResponse& redirect_response,
                                     bool cross_origin) {
  redirect_chain_.push_back(redirect_response);
  if (has_cross_origin_redirect_)
    return;
  if (cross_origin) {
    has_cross_origin_redirect_ = true;
    transfer_size_ = 0;
  } else {
    DCHECK_GE(redirect_response.EncodedDataLength(), 0);
    transfer_size_ += redirect_response.EncodedDataLength();
  }
}

}  // namespace blink
