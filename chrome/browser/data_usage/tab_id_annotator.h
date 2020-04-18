// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DATA_USAGE_TAB_ID_ANNOTATOR_H_
#define CHROME_BROWSER_DATA_USAGE_TAB_ID_ANNOTATOR_H_

#include <memory>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/data_usage/core/data_use_annotator.h"

namespace data_usage {
struct DataUse;
}

namespace net {
class URLRequest;
}

namespace chrome_browser_data_usage {

// Class that annotates DataUse objects with their associated tab IDs.
class TabIdAnnotator : public data_usage::DataUseAnnotator {
 public:
  TabIdAnnotator();
  ~TabIdAnnotator() override;

  // Determines the tab ID associated with |request| if there is one, setting
  // the tab ID on |data_use| appropriately and passing it to |callback| once
  // the tab ID is ready. A tab ID of -1 is used if no tab ID is found for
  // |request|. This method will attach a new TabIdProvider to |request| as user
  // data if there isn't one attached already and |request| has enough
  // information to get a tab ID.
  void Annotate(net::URLRequest* request,
                std::unique_ptr<data_usage::DataUse> data_use,
                const DataUseConsumerCallback& callback) override;

 private:
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(TabIdAnnotator);
};

}  // namespace chrome_browser_data_usage

#endif  // CHROME_BROWSER_DATA_USAGE_TAB_ID_ANNOTATOR_H_
