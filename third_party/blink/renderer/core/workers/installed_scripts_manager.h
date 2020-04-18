// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_INSTALLED_SCRIPTS_MANAGER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_INSTALLED_SCRIPTS_MANAGER_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/network/content_security_policy_response_headers.h"
#include "third_party/blink/renderer/platform/network/http_header_map.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// InstalledScriptsManager provides the scripts of workers that have been
// installed. Currently it is only used for installed service workers.
class InstalledScriptsManager {
 public:
  InstalledScriptsManager() = default;

  class CORE_EXPORT ScriptData {
   public:
    ScriptData() = default;
    ScriptData(const KURL& script_url,
               String source_text,
               std::unique_ptr<Vector<char>> meta_data,
               std::unique_ptr<CrossThreadHTTPHeaderMapData>);
    ScriptData(ScriptData&& other) = default;
    ScriptData& operator=(ScriptData&& other) = default;

    String TakeSourceText() { return std::move(source_text_); }
    std::unique_ptr<Vector<char>> TakeMetaData() {
      return std::move(meta_data_);
    }

    ContentSecurityPolicyResponseHeaders
    GetContentSecurityPolicyResponseHeaders();
    String GetReferrerPolicy();
    std::unique_ptr<Vector<String>> CreateOriginTrialTokens();

   private:
    KURL script_url_;
    String source_text_;
    std::unique_ptr<Vector<char>> meta_data_;
    HTTPHeaderMap headers_;

    DISALLOW_COPY_AND_ASSIGN(ScriptData);
  };

  // Used on the main or worker thread. Returns true if the script has been
  // installed.
  virtual bool IsScriptInstalled(const KURL& script_url) const = 0;

  enum class ScriptStatus { kSuccess, kFailed };
  // Used on the worker thread. GetScriptData() can provide a script for the
  // |script_url| only once. When GetScriptData returns
  // - ScriptStatus::kSuccess: the script has been received correctly. Sets
  //                           |out_script_data| to the script.
  // - ScriptStatus::kFailed: an error happened while receiving the script from
  //                          the browser process. |out_script_data| is set to
  //                          empty ScriptData.
  // This can block if the script has not been received from the browser process
  // yet.
  virtual ScriptStatus GetScriptData(const KURL& script_url,
                                     ScriptData* out_script_data) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_INSTALLED_SCRIPTS_MANAGER_H_
