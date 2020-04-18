// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_CLASSIC_SCRIPT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_CLASSIC_SCRIPT_H_

#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/loader/resource/script_resource.h"
#include "third_party/blink/renderer/core/script/script.h"
#include "third_party/blink/renderer/platform/loader/fetch/access_control_status.h"
#include "third_party/blink/renderer/platform/loader/fetch/script_fetch_options.h"

namespace blink {

class CORE_EXPORT ClassicScript final : public Script {
 public:
  static ClassicScript* Create(const ScriptSourceCode& script_source_code,
                               const KURL& base_url,
                               const ScriptFetchOptions& fetch_options,
                               AccessControlStatus access_control_status) {
    return new ClassicScript(script_source_code, base_url, fetch_options,
                             access_control_status);
  }

  void Trace(blink::Visitor*) override;

  const ScriptSourceCode& GetScriptSourceCode() const {
    return script_source_code_;
  }

 private:
  ClassicScript(const ScriptSourceCode& script_source_code,
                const KURL& base_url,
                const ScriptFetchOptions& fetch_options,
                AccessControlStatus access_control_status)
      : Script(fetch_options, base_url),
        script_source_code_(script_source_code),
        access_control_status_(access_control_status) {}

  ScriptType GetScriptType() const override { return ScriptType::kClassic; }
  void RunScript(LocalFrame*, const SecurityOrigin*) const override;
  String InlineSourceTextForCSP() const override {
    return script_source_code_.Source();
  }

  const ScriptSourceCode script_source_code_;
  const AccessControlStatus access_control_status_;
};

}  // namespace blink

#endif
