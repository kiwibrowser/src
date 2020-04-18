// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_JS_API_SERVICE_IMPL_H_
#define COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_JS_API_SERVICE_IMPL_H_

#include "base/macros.h"
#include "components/contextual_search/browser/contextual_search_js_api_handler.h"
#include "components/contextual_search/common/contextual_search_js_api_service.mojom.h"

namespace contextual_search {

// This is the receiving end of Contextual Search JavaScript API calls.
class ContextualSearchJsApiServiceImpl
    : public mojom::ContextualSearchJsApiService {
 public:
  explicit ContextualSearchJsApiServiceImpl(
      ContextualSearchJsApiHandler* contextual_search_js_api_handler);
  ~ContextualSearchJsApiServiceImpl() override;

  // Mojo ContextualSearchApiService implementation.
  // Determines if the JavaScript API should be enabled for the given |gurl|.
  // The given |callback| will be notified with the answer.
  void ShouldEnableJsApi(
      const GURL& gurl,
      contextual_search::mojom::ContextualSearchJsApiService::
          ShouldEnableJsApiCallback callback) override;

  // Handles a JavaScript call to set the caption in the Bar to
  // the given |message|.
  void HandleSetCaption(const std::string& message, bool does_answer) override;

 private:
  // The UI handler for calls through the JavaScript API.
  ContextualSearchJsApiHandler* contextual_search_js_api_handler_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSearchJsApiServiceImpl);
};

// static
void CreateContextualSearchJsApiService(
    ContextualSearchJsApiHandler* contextual_search_js_api_handler,
    mojom::ContextualSearchJsApiServiceRequest request);

}  // namespace contextual_search

#endif  // COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_JS_API_SERVICE_IMPL_H_
