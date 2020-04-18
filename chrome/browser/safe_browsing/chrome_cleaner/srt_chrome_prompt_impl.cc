// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/srt_chrome_prompt_impl.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"

namespace safe_browsing {

using chrome_cleaner::mojom::ChromePrompt;
using chrome_cleaner::mojom::ChromePromptRequest;
using chrome_cleaner::mojom::PromptAcceptance;
using content::BrowserThread;

ChromePromptImpl::ChromePromptImpl(ChromePromptRequest request,
                                   base::Closure on_connection_closed,
                                   OnPromptUser on_prompt_user)
    : binding_(this, std::move(request)),
      on_prompt_user_(std::move(on_prompt_user)) {
  DCHECK(on_prompt_user_);
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  binding_.set_connection_error_handler(std::move(on_connection_closed));
}

ChromePromptImpl::~ChromePromptImpl() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void ChromePromptImpl::PromptUser(
    const std::vector<base::FilePath>& files_to_delete,
    const base::Optional<std::vector<base::string16>>& registry_keys,
    ChromePrompt::PromptUserCallback callback) {
  using FileCollection = ChromeCleanerScannerResults::FileCollection;
  using RegistryKeyCollection =
      ChromeCleanerScannerResults::RegistryKeyCollection;

  if (on_prompt_user_) {
    ChromeCleanerScannerResults scanner_results(
        FileCollection(files_to_delete.begin(), files_to_delete.end()),
        registry_keys ? RegistryKeyCollection(registry_keys->begin(),
                                              registry_keys->end())
                      : RegistryKeyCollection());
    std::move(on_prompt_user_)
        .Run(std::move(scanner_results), std::move(callback));
  }
}

}  // namespace safe_browsing
