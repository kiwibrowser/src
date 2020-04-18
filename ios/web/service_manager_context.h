// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_SERVICE_MANAGER_CONTEXT_H_
#define IOS_WEB_SERVICE_MANAGER_CONTEXT_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace web {

class ServiceManagerConnection;

// ServiceManagerContext manages the browser's connection to the ServiceManager,
// hosting an in-process ServiceManagerContext.
class ServiceManagerContext {
 public:
  ServiceManagerContext();
  ~ServiceManagerContext();

 private:
  class InProcessServiceManagerContext;

  scoped_refptr<InProcessServiceManagerContext> in_process_context_;
  std::unique_ptr<ServiceManagerConnection> packaged_services_connection_;

  DISALLOW_COPY_AND_ASSIGN(ServiceManagerContext);
};

}  // namespace web

#endif  // IOS_WEB_SERVICE_MANAGER_CONTEXT_H_
