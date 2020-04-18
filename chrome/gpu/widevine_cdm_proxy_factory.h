// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_GPU_WIDEVINE_CDM_PROXY_FACTORY_H_
#define CHROME_GPU_WIDEVINE_CDM_PROXY_FACTORY_H_

#include <memory>

namespace media {
class CdmProxy;
}

std::unique_ptr<media::CdmProxy> CreateWidevineCdmProxy();

#endif  // CHROME_GPU_WIDEVINE_CDM_PROXY_FACTORY_H_
