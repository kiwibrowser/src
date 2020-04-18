// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_CRX_UPDATE_ITEM_H_
#define COMPONENTS_UPDATE_CLIENT_CRX_UPDATE_ITEM_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/version.h"
#include "components/update_client/crx_downloader.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"

namespace update_client {

struct CrxUpdateItem {
  CrxUpdateItem();
  CrxUpdateItem(const CrxUpdateItem& other);
  ~CrxUpdateItem();

  ComponentState state;

  std::string id;
  CrxComponent component;

  // Time when an update check for this CRX has happened.
  base::TimeTicks last_check;

  base::Version next_version;
  std::string next_fp;

  ErrorCategory error_category = ErrorCategory::kNone;
  int error_code = 0;
  int extra_code1 = 0;
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_CRX_UPDATE_ITEM_H_
