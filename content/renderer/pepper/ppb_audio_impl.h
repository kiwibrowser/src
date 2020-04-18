// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PPB_AUDIO_IMPL_H_
#define CONTENT_RENDERER_PEPPER_PPB_AUDIO_IMPL_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "base/sync_socket.h"
#include "content/public/renderer/plugin_instance_throttler.h"
#include "content/renderer/pepper/audio_helper.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/ppb_audio.h"
#include "ppapi/c/ppb_audio_config.h"
#include "ppapi/shared_impl/ppb_audio_config_shared.h"
#include "ppapi/shared_impl/ppb_audio_shared.h"
#include "ppapi/shared_impl/resource.h"
#include "ppapi/shared_impl/scoped_pp_resource.h"

namespace content {
class PepperPlatformAudioOutput;

// Some of the backend functionality of this class is implemented by the
// PPB_Audio_Shared so it can be shared with the proxy.
//
// TODO(teravest): PPB_Audio is no longer supported in-process. Clean this up
// to look more like typical HostResource implementations.
class PPB_Audio_Impl : public ppapi::Resource,
                       public ppapi::PPB_Audio_Shared,
                       public AudioHelper,
                       public PluginInstanceThrottler::Observer {
 public:
  explicit PPB_Audio_Impl(PP_Instance instance);

  // Resource overrides.
  ppapi::thunk::PPB_Audio_API* AsPPB_Audio_API() override;

  // PPB_Audio_API implementation.
  PP_Resource GetCurrentConfig() override;
  PP_Bool StartPlayback() override;
  PP_Bool StopPlayback() override;
  int32_t Open(PP_Resource config_id,
               scoped_refptr<ppapi::TrackedCallback> create_callback) override;
  int32_t GetSyncSocket(int* sync_socket) override;
  int32_t GetSharedMemory(base::SharedMemory** shm,
                          uint32_t* shm_size) override;

  void SetVolume(double volume);

 private:
  ~PPB_Audio_Impl() override;

  // AudioHelper implementation.
  void OnSetStreamInfo(base::SharedMemoryHandle shared_memory_handle,
                       size_t shared_memory_size_,
                       base::SyncSocket::Handle socket) override;

  // PluginInstanceThrottler::Observer implementation.
  void OnThrottleStateChange() override;

  // Starts the deferred playback and unsubscribes from the throttler.
  void StartDeferredPlayback();

  // AudioConfig used for creating this Audio object. We own a ref.
  ppapi::ScopedPPResource config_;

  // PluginDelegate audio object that we delegate audio IPC through. We don't
  // own this pointer but are responsible for calling Shutdown on it.
  PepperPlatformAudioOutput* audio_;

  // Stream is playing, but throttled due to Plugin Power Saver.
  bool playback_throttled_;

  DISALLOW_COPY_AND_ASSIGN(PPB_Audio_Impl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PPB_AUDIO_IMPL_H_
