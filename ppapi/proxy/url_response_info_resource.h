// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_URL_RESPONSE_INFO_RESOURCE_H_
#define PPAPI_PROXY_URL_RESPONSE_INFO_RESOURCE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/scoped_pp_resource.h"
#include "ppapi/shared_impl/url_response_info_data.h"
#include "ppapi/thunk/ppb_url_response_info_api.h"

namespace ppapi {
namespace proxy {

class PPAPI_PROXY_EXPORT URLResponseInfoResource
    : public PluginResource,
      public thunk::PPB_URLResponseInfo_API {
 public:
  // The file_ref_resource should be the body_as_file_ref host resource in the
  // |data| converted to a resource valid in the current process (if we're
  // downloading to a file; it will be 0 if we're not). A reference
  // is passed from the caller and is taken over by this object.
  URLResponseInfoResource(Connection connection,
                          PP_Instance instance,
                          const URLResponseInfoData& data,
                          PP_Resource file_ref_resource);
  ~URLResponseInfoResource() override;

  // Resource override.
  PPB_URLResponseInfo_API* AsPPB_URLResponseInfo_API() override;

  // PPB_URLResponseInfo_API implementation.
  PP_Var GetProperty(PP_URLResponseProperty property) override;
  PP_Resource GetBodyAsFileRef() override;

  const URLResponseInfoData& data() const { return data_; }

 private:
  URLResponseInfoData data_;

  // Non-zero when the load is being streamed to a file.
  ScopedPPResource body_as_file_ref_;

  DISALLOW_COPY_AND_ASSIGN(URLResponseInfoResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_URL_RESPONSE_INFO_RESOURCE_H_
