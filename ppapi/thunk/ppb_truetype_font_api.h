// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_TRUETYPE_FONT_API_H_
#define PPAPI_THUNK_PPB_TRUETYPE_FONT_API_H_

#include <stdint.h>

#include "base/memory/ref_counted.h"
#include "ppapi/c/dev/ppb_truetype_font_dev.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {

class TrackedCallback;

namespace thunk {

class PPAPI_THUNK_EXPORT PPB_TrueTypeFont_API {
 public:
  virtual ~PPB_TrueTypeFont_API() {}

  virtual int32_t Describe(PP_TrueTypeFontDesc_Dev* desc,
                           scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t GetTableTags(const PP_ArrayOutput& output,
                               scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t GetTable(uint32_t table,
                           int32_t offset,
                           int32_t max_data_length,
                           const PP_ArrayOutput& output,
                           scoped_refptr<TrackedCallback> callback) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_TRUETYPE_FONT_API_H_
