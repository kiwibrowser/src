// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_OLD_RENDER_FRAME_AUDIO_INPUT_STREAM_FACTORY_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_OLD_RENDER_FRAME_AUDIO_INPUT_STREAM_FACTORY_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "content/browser/media/media_devices_util.h"
#include "content/browser/renderer_host/media/audio_input_device_manager.h"
#include "content/browser/renderer_host/media/audio_input_stream_handle.h"
#include "content/browser/renderer_host/media/media_devices_manager.h"
#include "content/common/content_export.h"
#include "content/common/media/renderer_audio_input_stream_factory.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "media/audio/audio_input_delegate.h"
#include "media/mojo/interfaces/audio_logging.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {
class AudioParameters;
}  // namespace media

namespace content {

class MediaStreamManager;

// Handles a RendererAudioInputStreamFactory request for a render frame host,
// using the provided RendererAudioInputStreamFactoryContext. This class may
// be constructed on any thread, but must be used on the IO thread after that,
// and also destructed on the IO thread. It is being
// replaced by RenderFrameAudioInputStreamFactory, which forwards stream
// requests to the audio service (https://crbug.com/830493).
class CONTENT_EXPORT OldRenderFrameAudioInputStreamFactory
    : public mojom::RendererAudioInputStreamFactory {
 public:
  using CreateDelegateCallback =
      base::RepeatingCallback<std::unique_ptr<media::AudioInputDelegate>(
          AudioInputDeviceManager* audio_input_device_manager,
          media::mojom::AudioLogPtr audio_log,
          AudioInputDeviceManager::KeyboardMicRegistration
              keyboard_mic_registration,
          uint32_t shared_memory_count,
          int stream_id,
          int session_id,
          bool automatic_gain_control,
          const media::AudioParameters& parameters,
          media::AudioInputDelegate::EventHandler* event_handler)>;

  OldRenderFrameAudioInputStreamFactory(
      CreateDelegateCallback create_delegate_callback,
      MediaStreamManager* media_stream_manager,
      int render_process_id,
      int render_frame_id);

  ~OldRenderFrameAudioInputStreamFactory() override;

 private:
  using InputStreamSet = base::flat_set<std::unique_ptr<AudioInputStreamHandle>,
                                        base::UniquePtrComparator>;

  // mojom::RendererAudioInputStreamFactory implementation.
  void CreateStream(mojom::RendererAudioInputStreamFactoryClientPtr client,
                    int32_t session_id,
                    const media::AudioParameters& audio_params,
                    bool automatic_gain_control,
                    uint32_t shared_memory_count) override;

  void DoCreateStream(mojom::RendererAudioInputStreamFactoryClientPtr client,
                      int session_id,
                      const media::AudioParameters& audio_params,
                      bool automatic_gain_control,
                      uint32_t shared_memory_count,
                      AudioInputDeviceManager::KeyboardMicRegistration
                          keyboard_mic_registration);

  void AssociateInputAndOutputForAec(
      const base::UnguessableToken& input_stream_id,
      const std::string& output_device_id) override;

  void TranslateAndSetOutputDeviceForAec(
      const base::UnguessableToken& input_stream_id,
      const std::string& output_device_id,
      const MediaDeviceSaltAndOrigin& salt_and_origin,
      const MediaDeviceEnumeration& devices);

  void RemoveStream(AudioInputStreamHandle* input_stream);

  const CreateDelegateCallback create_delegate_callback_;
  MediaStreamManager* media_stream_manager_;
  const int render_process_id_;
  const int render_frame_id_;

  InputStreamSet streams_;
  int next_stream_id_ = 0;

  base::WeakPtrFactory<OldRenderFrameAudioInputStreamFactory> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OldRenderFrameAudioInputStreamFactory);
};

// This class is a convenient bundle of factory and binding.
// It can be created on any thread, but should be destroyed on the IO thread
// (hence the DeleteOnIOThread pointer).
class CONTENT_EXPORT RenderFrameAudioInputStreamFactoryHandle {
 public:
  static std::unique_ptr<RenderFrameAudioInputStreamFactoryHandle,
                         BrowserThread::DeleteOnIOThread>
  CreateFactory(OldRenderFrameAudioInputStreamFactory::CreateDelegateCallback
                    create_delegate_callback,
                MediaStreamManager* media_stream_manager,
                int render_process_id,
                int render_frame_id,
                mojom::RendererAudioInputStreamFactoryRequest request);

  ~RenderFrameAudioInputStreamFactoryHandle();

 private:
  RenderFrameAudioInputStreamFactoryHandle(
      OldRenderFrameAudioInputStreamFactory::CreateDelegateCallback
          create_delegate_callback,
      MediaStreamManager* media_stream_manager,
      int render_process_id,
      int render_frame_id);

  void Init(mojom::RendererAudioInputStreamFactoryRequest request);

  OldRenderFrameAudioInputStreamFactory impl_;
  mojo::Binding<mojom::RendererAudioInputStreamFactory> binding_;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameAudioInputStreamFactoryHandle);
};

using UniqueAudioInputStreamFactoryPtr =
    std::unique_ptr<RenderFrameAudioInputStreamFactoryHandle,
                    BrowserThread::DeleteOnIOThread>;

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_OLD_RENDER_FRAME_AUDIO_INPUT_STREAM_FACTORY_H_
