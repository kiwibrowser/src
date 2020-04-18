// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_USER_DATA_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_USER_DATA_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/supports_user_data.h"
#include "components/previews/core/previews_experiments.h"

namespace net {
class URLRequest;
}

namespace previews {

// A representation of previews information related to a navigation.
class PreviewsUserData : public base::SupportsUserData::Data {
 public:
  PreviewsUserData(uint64_t page_id);
  ~PreviewsUserData() override;

  // Makes a deep copy.
  std::unique_ptr<PreviewsUserData> DeepCopy() const;

  // Creates the data on |request| if it does not exist, and returns a pointer
  // to the data.
  static PreviewsUserData* Create(net::URLRequest* request, uint64_t page_id);

  // Gets the data from the request if it exists.
  static PreviewsUserData* GetData(const net::URLRequest& request);

  // A session unique ID related to this navigation.
  uint64_t page_id() const { return page_id_; }

  // Sets a data savings inflation percent value to use instead of the default
  // if there is a committed preview. Note that this is expected to be used for
  // specific preview types (such as NoScript) that don't have better data use
  // estimation information.
  void SetDataSavingsInflationPercent(int inflation_percent) {
    data_savings_inflation_percent_ = inflation_percent;
  }

  // Returns the data savings inflation percent to use for this navigation
  // instead of the default if it is not 0.
  int data_savings_inflation_percent() {
    return data_savings_inflation_percent_;
  }

  // Sets that the page load received the Cache-Control:no-transform
  // directive. Expected to be set upon receiving a committed response.
  void SetCacheControlNoTransformDirective() {
    cache_control_no_transform_directive_ = true;
  }

  // Returns whether the Cache-Control:no-transform directive has been
  // detected for the request. Should not be called prior to receiving
  // a committed response.
  bool cache_control_no_transform_directive() {
    return cache_control_no_transform_directive_;
  }

  // Sets the committed previews type. Should only be called once.
  void SetCommittedPreviewsType(previews::PreviewsType previews_type);

  // The committed previews type, if any. Otherwise PreviewsType::NONE.
  previews::PreviewsType committed_previews_type() const {
    return committed_previews_type_;
  }

  // Whether there is a committed previews type.
  bool HasCommittedPreviewsType() const {
    return committed_previews_type_ != previews::PreviewsType::NONE;
  }

 private:
  // A session unique ID related to this navigation.
  const uint64_t page_id_;
  // A previews data savings inflation percent for the navigation if not 0.
  int data_savings_inflation_percent_ = 0;
  // Whether the origin provided a no-transform directive.
  bool cache_control_no_transform_directive_ = false;

  // The committed previews type, if any.
  previews::PreviewsType committed_previews_type_ = PreviewsType::NONE;

  DISALLOW_COPY_AND_ASSIGN(PreviewsUserData);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_USER_DATA_H_
