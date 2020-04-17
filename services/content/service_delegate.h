// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CONTENT_SERVICE_DELEGATE_H_
#define SERVICES_CONTENT_SERVICE_DELEGATE_H_

#include "services/content/public/mojom/view.mojom.h"

namespace content {

class Service;
class ViewDelegate;

// This is a delegate interface which allows the Content Service implementation
// to delegate out to private src/content code without a circular dependency
// between them.
//
// This interface is strictly intended to host transitional APIs for aiding in
// incremental conversion of src/content and its dependents into/onto the
// Content Service. As such, APIs should only be added here with a plan for
// eventual removal.
class ServiceDelegate {
 public:
  virtual ~ServiceDelegate() {}

  // Called when an instance of Service (specifically one using this
  // delegate) is about to be destroyed.
  virtual void WillDestroyServiceInstance(Service* service) = 0;

  // Constructs a new ViewDelegate implementation to back a new ContentViewImpl
  // instance, servicing a client's ContentView. |client| is a ViewClient
  // interface the implementation can use to communicate with the client of this
  // view.
  virtual std::unique_ptr<ViewDelegate> CreateViewDelegate(
      mojom::ViewClient* client) = 0;
};

};  // namespace content

#endif  // SERVICES_CONTENT_CONTENT_SERVICE_DELEGATE_H_
