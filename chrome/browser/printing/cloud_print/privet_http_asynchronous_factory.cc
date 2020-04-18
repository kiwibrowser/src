// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/privet_http_asynchronous_factory.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "chrome/browser/printing/cloud_print/privet_http_asynchronous_factory_impl.h"

namespace cloud_print {

// static
std::unique_ptr<PrivetHTTPAsynchronousFactory>
PrivetHTTPAsynchronousFactory::CreateInstance(
    net::URLRequestContextGetter* request_context) {
  return base::WrapUnique<PrivetHTTPAsynchronousFactory>(
      new PrivetHTTPAsynchronousFactoryImpl(request_context));
}

}  // namespace cloud_print
