// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_JS_API_HANDLER_H_
#define COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_JS_API_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "components/contextual_search/common/contextual_search_js_api_service.mojom.h"

namespace contextual_search {

// Interface that the Contextual Search Renderer uses to call back to
// the browser to handle its JavaScript API.
class ContextualSearchJsApiHandler {
 public:
  ContextualSearchJsApiHandler() {}
  virtual ~ContextualSearchJsApiHandler() {}

  // Enabling API, determines if the JS API should be enabled for the given URL.
  virtual void ShouldEnableJsApi(
      const GURL& gurl,
      contextual_search::mojom::ContextualSearchJsApiService::
          ShouldEnableJsApiCallback callback) = 0;

  //=======
  // JS API
  //=======

  // Set the caption in the Contextual Search Bar, and indicate whether
  // the caption provides an answer (such as an actual definition), rather than
  // just general notification of what kind of answer may be available.
  virtual void SetCaption(std::string caption, bool does_answer) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContextualSearchJsApiHandler);
};

}  // namespace contextual_search

#endif  // COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_JS_API_HANDLER_H_
