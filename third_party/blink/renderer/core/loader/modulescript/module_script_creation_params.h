// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_SCRIPT_CREATION_PARAMS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_SCRIPT_CREATION_PARAMS_H_

#include "base/optional.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/platform/cross_thread_copier.h"
#include "third_party/blink/renderer/platform/loader/fetch/access_control_status.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// ModuleScriptCreationParams contains parameters for creating ModuleScript.
class ModuleScriptCreationParams {
 public:
  ModuleScriptCreationParams(
      const KURL& response_url,
      const String& source_text,
      network::mojom::FetchCredentialsMode fetch_credentials_mode,
      AccessControlStatus access_control_status)
      : response_url_(response_url),
        source_text_(source_text),
        fetch_credentials_mode_(fetch_credentials_mode),
        access_control_status_(access_control_status) {}
  ~ModuleScriptCreationParams() = default;

  const KURL& GetResponseUrl() const { return response_url_; };
  const String& GetSourceText() const { return source_text_; }
  network::mojom::FetchCredentialsMode GetFetchCredentialsMode() const {
    return fetch_credentials_mode_;
  }
  AccessControlStatus GetAccessControlStatus() const {
    return access_control_status_;
  }

 private:
  const KURL response_url_;
  const String source_text_;
  const network::mojom::FetchCredentialsMode fetch_credentials_mode_;
  const AccessControlStatus access_control_status_;
};

// Creates a deep copy because |response_url_| and |source_text_| are not
// cross-thread-transfer-safe.
template <>
struct CrossThreadCopier<ModuleScriptCreationParams> {
  static ModuleScriptCreationParams Copy(
      const ModuleScriptCreationParams& params) {
    return ModuleScriptCreationParams(
        params.GetResponseUrl().Copy(), params.GetSourceText().IsolatedCopy(),
        params.GetFetchCredentialsMode(), params.GetAccessControlStatus());
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_SCRIPT_CREATION_PARAMS_H_
