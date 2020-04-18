// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_MANAGER_SERVICE_MANAGER_CONTEXT_H_
#define CONTENT_BROWSER_SERVICE_MANAGER_SERVICE_MANAGER_CONTEXT_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"

namespace service_manager {
class Connector;
}

namespace content {

class ServiceManagerConnection;

// ServiceManagerContext manages the browser's connection to the ServiceManager,
// hosting a new in-process ServiceManagerContext if the browser was not
// launched from an external one.
class CONTENT_EXPORT ServiceManagerContext {
 public:
  ServiceManagerContext();
  ~ServiceManagerContext();

  // Returns a service_manager::Connector that can be used on the IO thread.
  static service_manager::Connector* GetConnectorForIOThread();

  // Returns true if there is a valid process for |process_group_name|. Must be
  // called on the IO thread.
  static bool HasValidProcessForProcessGroup(
      const std::string& process_group_name);

 private:
  class InProcessServiceManagerContext;

  scoped_refptr<InProcessServiceManagerContext> in_process_context_;
  std::unique_ptr<ServiceManagerConnection> packaged_services_connection_;

  DISALLOW_COPY_AND_ASSIGN(ServiceManagerContext);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_MANAGER_SERVICE_MANAGER_CONTEXT_H_
