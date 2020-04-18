// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_EMBEDDER_PROCESS_TYPE_H_
#define SERVICES_SERVICE_MANAGER_EMBEDDER_PROCESS_TYPE_H_

namespace service_manager {

enum class ProcessType {
  // An unspecified process type. If this is given anywhere a ProcessType is
  // expected, it must be interpreted as some reasonable default based on
  // context.
  kDefault,

  // A standalone Service Manager process. There can be only one.
  kServiceManager,

  // A service process. A service process hosts one or more embedded service
  // instances.
  kService,

  // An embedder process. The Service Manager implementation does not control
  // any aspect of the process's logic beyond primitive process initialization
  // and shutdown.
  kEmbedder,
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_EMBEDDER_PROCESS_TYPE_H_
