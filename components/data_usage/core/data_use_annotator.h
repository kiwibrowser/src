// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_USAGE_CORE_DATA_USE_ANNOTATOR_H_
#define COMPONENTS_DATA_USAGE_CORE_DATA_USE_ANNOTATOR_H_

#include <memory>

#include "base/callback.h"

namespace net {
class URLRequest;
}

namespace data_usage {

struct DataUse;

// Interface for an object that can annotate DataUse objects with additional
// information.
class DataUseAnnotator {
 public:
  typedef base::Callback<void(std::unique_ptr<DataUse>)>
      DataUseConsumerCallback;

  virtual ~DataUseAnnotator() {}

  // Annotate |data_use| with additional information, possibly from |request|,
  // before passing the annotated |data_use| to |callback|. |request| is passed
  // in as a non-const pointer so that the DataUseAnnotator can add UserData on
  // to the |request| if desired.
  virtual void Annotate(net::URLRequest* request,
                        std::unique_ptr<DataUse> data_use,
                        const DataUseConsumerCallback& callback) = 0;
};

}  // namespace data_usage

#endif  // COMPONENTS_DATA_USAGE_CORE_DATA_USE_ANNOTATOR_H_
