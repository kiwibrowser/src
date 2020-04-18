// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/compositor_resource.h"

#include "base/logging.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/thunk/enter.h"

namespace ppapi {
namespace proxy {

CompositorResource::CompositorResource(Connection connection,
                                       PP_Instance instance)
    : PluginResource(connection, instance),
      layer_reset_(true),
      last_resource_id_(0) {
  SendCreate(RENDERER, PpapiHostMsg_Compositor_Create());
}

bool CompositorResource::IsInProgress() const {
  ProxyLock::AssertAcquiredDebugOnly();
  return TrackedCallback::IsPending(commit_callback_);
}

int32_t CompositorResource::GenerateResourceId() const {
  ProxyLock::AssertAcquiredDebugOnly();
  return ++last_resource_id_;
}

CompositorResource::~CompositorResource() {
  ResetLayersInternal(true);

  // Abort all release callbacks.
  for (ReleaseCallbackMap::iterator it = release_callback_map_.begin();
       it != release_callback_map_.end(); ++it) {
    if (!it->second.is_null())
      it->second.Run(PP_ERROR_ABORTED, gpu::SyncToken(), false);
  }
}

thunk::PPB_Compositor_API* CompositorResource::AsPPB_Compositor_API() {
  return this;
}

void CompositorResource::OnReplyReceived(
    const ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
   PPAPI_BEGIN_MESSAGE_MAP(CompositorResource, msg)
     PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
         PpapiPluginMsg_Compositor_ReleaseResource,
         OnPluginMsgReleaseResource)
     PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL_UNHANDLED(
          PluginResource::OnReplyReceived(params, msg))
   PPAPI_END_MESSAGE_MAP()
}

PP_Resource CompositorResource::AddLayer() {
  scoped_refptr<CompositorLayerResource> resource(new CompositorLayerResource(
      connection(), pp_instance(), this));
  layers_.push_back(resource);
  return resource->GetReference();
}

int32_t CompositorResource::CommitLayers(
    const scoped_refptr<ppapi::TrackedCallback>& callback) {
  if (IsInProgress())
    return PP_ERROR_INPROGRESS;

  std::vector<CompositorLayerData> layers;
  layers.reserve(layers_.size());

  for (LayerList::const_iterator it = layers_.begin();
       it != layers_.end(); ++it) {
    if ((*it)->data().is_null())
      return PP_ERROR_FAILED;
    layers.push_back((*it)->data());
  }

  commit_callback_ = callback;
  Call<PpapiPluginMsg_Compositor_CommitLayersReply>(
      RENDERER,
      PpapiHostMsg_Compositor_CommitLayers(layers, layer_reset_),
      base::Bind(&CompositorResource::OnPluginMsgCommitLayersReply,
                 base::Unretained(this)),
      callback);

  return PP_OK_COMPLETIONPENDING;
}

int32_t CompositorResource::ResetLayers() {
  if (IsInProgress())
    return PP_ERROR_INPROGRESS;

  ResetLayersInternal(false);
  return PP_OK;
}

void CompositorResource::OnPluginMsgCommitLayersReply(
    const ResourceMessageReplyParams& params) {
  if (!TrackedCallback::IsPending(commit_callback_))
    return;

  // On success, we put layers' release_callbacks into a map,
  // otherwise we will do nothing. So plugin may change layers and
  // call CommitLayers() again.
  if (params.result() == PP_OK) {
    layer_reset_ = false;
    for (LayerList::iterator it = layers_.begin();
         it != layers_.end(); ++it) {
      ReleaseCallback release_callback = (*it)->release_callback();
      if (!release_callback.is_null()) {
        release_callback_map_.insert(ReleaseCallbackMap::value_type(
            (*it)->data().common.resource_id, release_callback));
        (*it)->ResetReleaseCallback();
      }
    }
  }

  scoped_refptr<TrackedCallback> callback;
  callback.swap(commit_callback_);
  callback->Run(params.result());
}

void CompositorResource::OnPluginMsgReleaseResource(
    const ResourceMessageReplyParams& params,
    int32_t id,
    const gpu::SyncToken& sync_token,
    bool is_lost) {
  ReleaseCallbackMap::iterator it = release_callback_map_.find(id);
  DCHECK(it != release_callback_map_.end()) <<
      "Can not found release_callback_ by id(" << id << ")!";
  it->second.Run(PP_OK, sync_token, is_lost);
  release_callback_map_.erase(it);
}

void CompositorResource::ResetLayersInternal(bool is_aborted) {
  for (LayerList::iterator it = layers_.begin();
       it != layers_.end(); ++it) {
    ReleaseCallback release_callback = (*it)->release_callback();
    if (!release_callback.is_null()) {
      release_callback.Run(is_aborted ? PP_ERROR_ABORTED : PP_OK,
                           gpu::SyncToken(), false);
      (*it)->ResetReleaseCallback();
    }
    (*it)->Invalidate();
  }

  layers_.clear();
  layer_reset_ = true;
}

}  // namespace proxy
}  // namespace ppapi
