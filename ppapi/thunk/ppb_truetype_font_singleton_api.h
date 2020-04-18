// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_TRUETYPE_FONT_SINGLETON_API_H_
#define PPAPI_THUNK_PPB_TRUETYPE_FONT_SINGLETON_API_H_

#include <stdint.h>

#include "ppapi/c/pp_array_output.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/shared_impl/singleton_resource_id.h"
#include "ppapi/shared_impl/tracked_callback.h"

namespace ppapi {
namespace thunk {

class PPB_TrueTypeFont_Singleton_API {
 public:
  virtual ~PPB_TrueTypeFont_Singleton_API() {}

  virtual int32_t GetFontFamilies(
      PP_Instance instance,
      const PP_ArrayOutput& output,
      const scoped_refptr<TrackedCallback>& callback) = 0;

  virtual int32_t GetFontsInFamily(
      PP_Instance instance,
      PP_Var family,
      const PP_ArrayOutput& output,
      const scoped_refptr<TrackedCallback>& callback) = 0;

  static const SingletonResourceID kSingletonResourceID =
      TRUETYPE_FONT_SINGLETON_ID;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_TRUETYPE_FONT_SINGLETON_API_H_
