// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TRUETYPE_FONT_LIST_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TRUETYPE_FONT_LIST_HOST_H_

#include "base/macros.h"
#include "ppapi/host/resource_host.h"

namespace content {

class BrowserPpapiHost;

class PepperTrueTypeFontListHost : public ppapi::host::ResourceHost {
 public:
  PepperTrueTypeFontListHost(BrowserPpapiHost* host,
                             PP_Instance instance,
                             PP_Resource resource);
  ~PepperTrueTypeFontListHost() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PepperTrueTypeFontListHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TRUETYPE_FONT_LIST_HOST_H_
