// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/launcher_search_provider.h"

#include <memory>
#include <utility>

#include "chrome/browser/chromeos/launcher_search_provider/error_reporter.h"
#include "chrome/browser/chromeos/launcher_search_provider/launcher_search_provider_service.h"
#include "chrome/common/extensions/api/launcher_search_provider.h"
#include "content/public/browser/render_frame_host.h"

namespace extensions {

LauncherSearchProviderSetSearchResultsFunction::
    ~LauncherSearchProviderSetSearchResultsFunction() {
}

ExtensionFunction::ResponseAction
LauncherSearchProviderSetSearchResultsFunction::Run() {
  using chromeos::launcher_search_provider::ErrorReporter;
  using chromeos::launcher_search_provider::Service;
  using extensions::api::launcher_search_provider::SetSearchResults::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);
  DCHECK(render_frame_host());

  std::unique_ptr<ErrorReporter> error_reporter(
      new ErrorReporter(render_frame_host()));
  Service* const service = Service::Get(browser_context());
  service->SetSearchResults(extension(), std::move(error_reporter),
                            params->query_id, params->results);

  return RespondNow(NoArguments());
}

}  // namespace extensions
