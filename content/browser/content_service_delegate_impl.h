// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CONTENT_SERVICE_DELEGATE_IMPL_H_
#define CONTENT_BROWSER_CONTENT_SERVICE_DELEGATE_IMPL_H_

#include <memory>
#include <set>

#include "base/macros.h"
#include "services/content/service_delegate.h"

namespace content {

class BrowserContext;

// Implementation of the main delegate interface for the Content Service. This
// is used to support the Content Service implementation with content/browser
// details, without the Content Service having any build dependencies on
// src/content. There is one instance of this delegate per BrowserContext,
// shared by any ContentService instance instantiated on behalf of that
// BrowserContext.
class ContentServiceDelegateImpl : public content::ServiceDelegate {
 public:
  // Constructs a new ContentServiceDelegateImpl for |browser_context|.
  // |browser_context| must outlive |this|.
  explicit ContentServiceDelegateImpl(BrowserContext* browser_context);
  ~ContentServiceDelegateImpl() override;

  // Registers |service| with this delegate. Must be called for any |service|
  // using |this| as its ContentServiceDelegate. Automatically balanced by
  // |WillDestroyServiceInstance()|.
  void AddService(content::Service* service);

 private:
  // content::ContentServiceDelegate:
  void WillDestroyServiceInstance(content::Service* service) override;
  std::unique_ptr<ViewDelegate> CreateViewDelegate(
      mojom::ViewClient* client) override;

  BrowserContext* const browser_context_;

  // Tracks ContentService instances currently using this delegate. Necessary
  // because the lifetime of |this| is tied to the lifetime of
  // |browser_context_|; on destruction of |this|, we need to force all of these
  // ContentService instances to terminate, since they cannot operate without
  // their delegate.
  std::set<content::Service*> service_instances_;

  DISALLOW_COPY_AND_ASSIGN(ContentServiceDelegateImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CONTENT_SERVICE_DELEGATE_IMPL_H_
