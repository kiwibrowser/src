// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PEPPER_AUDIO_OUTPUT_HOST_H_
#define CONTENT_RENDERER_PEPPER_PEPPER_AUDIO_OUTPUT_HOST_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/shared_memory.h"
#include "base/sync_socket.h"
#include "content/public/renderer/plugin_instance_throttler.h"
#include "content/renderer/pepper/pepper_device_enumeration_host_helper.h"
#include "ipc/ipc_platform_file.h"
#include "ppapi/c/ppb_audio_config.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/resource_host.h"

namespace content {
class PepperPlatformAudioOutputDev;
class RendererPpapiHostImpl;

class PepperAudioOutputHost : public ppapi::host::ResourceHost,
                              public PluginInstanceThrottler::Observer {
 public:
  PepperAudioOutputHost(RendererPpapiHostImpl* host,
                        PP_Instance instance,
                        PP_Resource resource);
  ~PepperAudioOutputHost() override;

  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

  // Called when the stream is created.
  void StreamCreated(base::SharedMemoryHandle shared_memory_handle,
                     size_t shared_memory_size,
                     base::SyncSocket::Handle socket);
  void StreamCreationFailed();
  void SetVolume(double volume);

 private:
  int32_t OnOpen(ppapi::host::HostMessageContext* context,
                 const std::string& device_id,
                 PP_AudioSampleRate sample_rate,
                 uint32_t sample_frame_count);
  int32_t OnStartOrStop(ppapi::host::HostMessageContext* context,
                        bool playback);
  int32_t OnClose(ppapi::host::HostMessageContext* context);

  void OnOpenComplete(int32_t result,
                      base::SharedMemoryHandle shared_memory_handle,
                      size_t shared_memory_size,
                      base::SyncSocket::Handle socket_handle);

  int32_t GetRemoteHandles(
      const base::SyncSocket& socket,
      const base::SharedMemory& shared_memory,
      IPC::PlatformFileForTransit* remote_socket_handle,
      base::SharedMemoryHandle* remote_shared_memory_handle);

  void Close();

  void SendOpenReply(int32_t result);

  // PluginInstanceThrottler::Observer implementation.
  void OnThrottleStateChange() override;

  // Starts the deferred playback and unsubscribes from the throttler.
  void StartDeferredPlayback();

  // Non-owning pointer.
  RendererPpapiHostImpl* renderer_ppapi_host_;

  ppapi::host::ReplyMessageContext open_context_;

  // Audio output object that we delegate audio IPC through.
  // We don't own this pointer but are responsible for calling Shutdown on it.
  PepperPlatformAudioOutputDev* audio_output_;

  // Stream is playing, but throttled due to Plugin Power Saver.
  bool playback_throttled_;

  PepperDeviceEnumerationHostHelper enumeration_helper_;

  DISALLOW_COPY_AND_ASSIGN(PepperAudioOutputHost);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PEPPER_AUDIO_OUTPUT_HOST_H_
