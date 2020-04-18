// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_

#include <stdint.h>

#include <map>
#include <string>

#include "base/scoped_observer.h"
#include "content/public/common/transferrable_url_loader.mojom.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_function.h"
#include "extensions/browser/extension_registry_observer.h"

namespace content {
class BrowserContext;
class StreamHandle;
struct StreamInfo;
}

namespace extensions {
class ExtensionRegistry;

class StreamsPrivateAPI : public BrowserContextKeyedAPI,
                          public ExtensionRegistryObserver {
 public:
  // Send the onExecuteMimeTypeHandler event to |extension_id|.
  // The data for the document will be readable from |stream|, and
  // should be |expected_content_size| bytes long. If the viewer is being opened
  // in a BrowserPlugin, specify a non-empty |view_id| of the plugin. |embedded|
  // should be set to whether the document is embedded within another document.
  // The |frame_tree_node_id| parameter is used for PlzNavigate for the top
  // level plugins case. (PDF, etc). If this parameter has a valid value then
  // it overrides the |render_process_id| and |render_frame_id| parameters.
  // The |render_process_id| is the id of the renderer process.
  // The |render_frame_id| is the routing id of the RenderFrameHost.
  //
  // If the network service is not enabled, |stream| is used; otherwise,
  // |transferrable_loader| and |original_url| are used instead.
  static void SendExecuteMimeTypeHandlerEvent(
      int64_t expected_content_size,
      const std::string& extension_id,
      const std::string& view_id,
      bool embedded,
      int frame_tree_node_id,
      int render_process_id,
      int render_frame_id,
      std::unique_ptr<content::StreamInfo> stream,
      content::mojom::TransferrableURLLoaderPtr transferrable_loader,
      const GURL& original_url);

  explicit StreamsPrivateAPI(content::BrowserContext* context);
  ~StreamsPrivateAPI() override;

  void AbortStream(const std::string& extension_id,
                   const GURL& stream_url,
                   const base::Closure& callback);

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<StreamsPrivateAPI>* GetFactoryInstance();

 private:
  friend class BrowserContextKeyedAPIFactory<StreamsPrivateAPI>;

  // ExtensionRegistryObserver implementation.
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() {
    return "StreamsPrivateAPI";
  }
  static const bool kServiceIsNULLWhileTesting = true;
  static const bool kServiceRedirectedInIncognito = true;

  content::BrowserContext* const browser_context_;
  using StreamMap =
      std::map<std::string,
               std::map<GURL, std::unique_ptr<content::StreamHandle>>>;
  StreamMap streams_;

  // Listen to extension unloaded notifications.
  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;

  base::WeakPtrFactory<StreamsPrivateAPI> weak_ptr_factory_;

};

class StreamsPrivateAbortFunction : public UIThreadExtensionFunction {
 public:
  StreamsPrivateAbortFunction();
  DECLARE_EXTENSION_FUNCTION("streamsPrivate.abort", STREAMSPRIVATE_ABORT)

 protected:
  ~StreamsPrivateAbortFunction() override {}

  // ExtensionFunction:
  ExtensionFunction::ResponseAction Run() override;

 private:
  void OnClose();

  std::string stream_url_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_
