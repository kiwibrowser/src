// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_COMPOSITOR_RESOURCE_H_
#define PPAPI_PROXY_COMPOSITOR_RESOURCE_H_

#include <stdint.h>

#include <map>

#include "base/macros.h"
#include "ppapi/proxy/compositor_layer_resource.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/proxy_lock.h"
#include "ppapi/thunk/ppb_compositor_api.h"

namespace gpu {
struct SyncToken;
}

namespace ppapi {
namespace proxy {

class PPAPI_PROXY_EXPORT CompositorResource
    : public PluginResource,
      public thunk::PPB_Compositor_API {
 public:
  CompositorResource(Connection connection,
                     PP_Instance instance);

  bool IsInProgress() const;

  int32_t GenerateResourceId() const;

 private:
  ~CompositorResource() override;

  // Resource overrides:
  thunk::PPB_Compositor_API* AsPPB_Compositor_API() override;

  // PluginResource overrides:
  void OnReplyReceived(const ResourceMessageReplyParams& params,
                       const IPC::Message& msg) override;

  // thunk::PPB_Compositor_API overrides:
  PP_Resource AddLayer() override;
  int32_t CommitLayers(const scoped_refptr<TrackedCallback>& callback) override;
  int32_t ResetLayers() override;

  // IPC msg handlers:
  void OnPluginMsgCommitLayersReply(const ResourceMessageReplyParams& params);
  void OnPluginMsgReleaseResource(const ResourceMessageReplyParams& params,
                                  int32_t id,
                                  const gpu::SyncToken& sync_token,
                                  bool is_lost);

  void ResetLayersInternal(bool is_aborted);

  // Callback for CommitLayers().
  scoped_refptr<TrackedCallback> commit_callback_;

  // True if layers_ has been reset by ResetLayers().
  bool layer_reset_;

  // Layer stack.
  typedef std::vector<scoped_refptr<CompositorLayerResource> > LayerList;
  LayerList layers_;

  // Release callback map for texture and image.
  typedef CompositorLayerResource::ReleaseCallback ReleaseCallback;
  typedef std::map<int32_t, ReleaseCallback> ReleaseCallbackMap;
  ReleaseCallbackMap release_callback_map_;

  // The last resource id for texture or image.
  mutable int32_t last_resource_id_;

  DISALLOW_COPY_AND_ASSIGN(CompositorResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_COMPOSITOR_RESOURCE_H_
