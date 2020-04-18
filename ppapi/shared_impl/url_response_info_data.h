// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_URL_RESPONSE_INFO_DATA_H_
#define PPAPI_SHARED_IMPL_URL_RESPONSE_INFO_DATA_H_

#include <string>

#include "ppapi/c/pp_stdint.h"
#include "ppapi/shared_impl/file_ref_create_info.h"
#include "ppapi/shared_impl/ppapi_shared_export.h"

namespace ppapi {

struct PPAPI_SHARED_EXPORT URLResponseInfoData {
  URLResponseInfoData();
  ~URLResponseInfoData();

  std::string url;
  std::string headers;
  int32_t status_code;
  std::string status_text;
  std::string redirect_url;

  // Valid when streaming to a file.
  FileRefCreateInfo body_as_file_ref;
};

}  // namespace ppapi

#endif  // PPAPI_SHARED_IMPL_URL_RESPONSE_INFO_DATA_H_
