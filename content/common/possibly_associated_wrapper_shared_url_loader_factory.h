// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_POSSIBLY_ASSOCIATED_WRAPPER_SHARED_URL_LOADER_FACTORY_H_
#define CONTENT_COMMON_POSSIBLY_ASSOCIATED_WRAPPER_SHARED_URL_LOADER_FACTORY_H_

#include "content/common/possibly_associated_interface_ptr.h"
#include "services/network/public/cpp/wrapper_shared_url_loader_factory.h"

namespace content {

using PossiblyAssociatedWrapperSharedURLLoaderFactory =
    network::WrapperSharedURLLoaderFactoryBase<PossiblyAssociatedInterfacePtr>;

}  // namespace content

#endif  // CONTENT_COMMON_POSSIBLY_ASSOCIATED_WRAPPER_SHARED_URL_LOADER_FACTORY_H_
