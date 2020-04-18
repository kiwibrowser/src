// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_EXTENSIONS_WEBSTORE_BINDINGS_H_
#define CHROME_RENDERER_EXTENSIONS_WEBSTORE_BINDINGS_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/common/extensions/mojom/inline_install.mojom.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "v8/include/v8.h"

namespace blink {
class WebLocalFrame;
}

namespace extensions {
class ScriptContext;

// A V8 extension that creates an object at window.chrome.webstore. This object
// allows JavaScript to initiate inline installs of apps that are listed in the
// Chrome Web Store (CWS).
class WebstoreBindings : public ObjectBackedNativeHandler,
                         public mojom::InlineInstallProgressListener {
 public:
  explicit WebstoreBindings(ScriptContext* context);
  ~WebstoreBindings() override;

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

  // mojom::InlineInstallProgressListener:
  void InlineInstallStageChanged(api::webstore::InstallStage stage) override;
  void InlineInstallDownloadProgress(int percent_downloaded) override;

 private:
  void InlineInstallResponse(int install_id,
                             bool success,
                             const std::string& error,
                             webstore_install::Result result);
  void Install(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Extracts a Web Store item ID from a <link rel="chrome-webstore-item"
  // href="https://chrome.google.com/webstore/detail/id"> node found in the
  // frame. On success, true will be returned and the |webstore_item_id|
  // parameter will be populated with the ID. On failure, false will be returned
  // and |error| will be populated with the error.
  static bool GetWebstoreItemIdFromFrame(
      blink::WebLocalFrame* frame,
      const std::string& preferred_store_link_url,
      std::string* webstore_item_id,
      std::string* error);

  // ObjectBackedNativeHandler:
  void Invalidate() override;

  mojom::InlineInstallerAssociatedPtr inline_installer_;

  mojo::BindingSet<mojom::InlineInstallProgressListener>
      install_progress_listener_bindings_;

  DISALLOW_COPY_AND_ASSIGN(WebstoreBindings);
};

}  // namespace extensions

#endif  // CHROME_RENDERER_EXTENSIONS_WEBSTORE_BINDINGS_H_
