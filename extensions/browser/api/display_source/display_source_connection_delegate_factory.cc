// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/display_source/display_source_connection_delegate_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extensions_browser_client.h"

namespace extensions {

using content::BrowserContext;

// static
DisplaySourceConnectionDelegate*
DisplaySourceConnectionDelegateFactory::GetForBrowserContext(
    BrowserContext* browser_context) {
  return static_cast<DisplaySourceConnectionDelegate*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

// static
DisplaySourceConnectionDelegateFactory*
DisplaySourceConnectionDelegateFactory::GetInstance() {
  return base::Singleton<DisplaySourceConnectionDelegateFactory>::get();
}

DisplaySourceConnectionDelegateFactory::DisplaySourceConnectionDelegateFactory()
    : BrowserContextKeyedServiceFactory(
          "DisplaySourceConnectionDelegate",
          BrowserContextDependencyManager::GetInstance()) {}

DisplaySourceConnectionDelegateFactory::
    ~DisplaySourceConnectionDelegateFactory() {}

KeyedService* DisplaySourceConnectionDelegateFactory::BuildServiceInstanceFor(
    BrowserContext* browser_context) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DisplaySourceConnectionDelegate* delegate = nullptr;
  // TODO(mikhail): Add implementation.
  return delegate;
}

BrowserContext* DisplaySourceConnectionDelegateFactory::GetBrowserContextToUse(
    BrowserContext* context) const {
  return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
}

bool DisplaySourceConnectionDelegateFactory::
    ServiceIsCreatedWithBrowserContext() const {
  return false;
}

bool DisplaySourceConnectionDelegateFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace extensions
