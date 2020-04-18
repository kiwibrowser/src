// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_CONNECTION_FILTER_H_
#define CONTENT_PUBLIC_COMMON_CONNECTION_FILTER_H_

#include "content/common/content_export.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace service_manager {
class Connector;
struct BindSourceInfo;
}

namespace content {

// TODO(beng): Rename to InterfaceBinder?
// See ServiceManagerConnection::AddConnectionFilter().
class CONTENT_EXPORT ConnectionFilter {
 public:
  virtual ~ConnectionFilter() {}

  // Provides the ConnectionFilter with the option of binding an interface
  // request from a remote service. The interface request is in
  // |interface_pipe|, which is taken by the binding action. If the interface
  // request is bound, subsequent ConnectionFilters are not consulted.
  virtual void OnBindInterface(
      const service_manager::BindSourceInfo& source_info,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe,
      service_manager::Connector* connector) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_CONNECTION_FILTER_H_
