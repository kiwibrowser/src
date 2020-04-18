// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_I18N_HOOKS_UTIL_H_
#define EXTENSIONS_RENDERER_I18N_HOOKS_UTIL_H_

#include <string>

#include "v8/include/v8.h"

namespace content {
class RenderFrame;
}

namespace extensions {
namespace i18n_hooks {

// Returns the localized method for the given |message_name| and
// substitutions. This can result in a synchronous IPC being sent to the browser
// for the first call related to an extension in this process.
v8::Local<v8::Value> GetI18nMessage(const std::string& message_name,
                                    const std::string& extension_id,
                                    v8::Local<v8::Value> v8_substitutions,
                                    content::RenderFrame* render_frame,
                                    v8::Local<v8::Context> context);

// Returns the detected language for the sample |text|.
v8::Local<v8::Value> DetectTextLanguage(v8::Local<v8::Context> context,
                                        const std::string& text);

}  // namespace i18n_hooks
}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_I18N_HOOKS_UTIL_H_
