// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_FORWARDING_AUDIO_STREAM_FACTORY_H_
#define CONTENT_BROWSER_MEDIA_FORWARDING_AUDIO_STREAM_FACTORY_H_

#include <memory>
#include <string>

#include "base/containers/flat_set.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/unguessable_token.h"
#include "content/browser/media/audio_muting_session.h"
#include "content/browser/media/audio_stream_broker.h"
#include "content/common/content_export.h"
#include "content/common/media/renderer_audio_input_stream_factory.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/audio/public/mojom/stream_factory.mojom.h"

namespace service_manager {
class Connector;
}

namespace media {
class AudioParameters;
}

namespace content {

class AudioStreamBroker;
class RenderFrameHost;
class WebContents;

// This class handles stream creation operations for a WebContents.
// This class is operated on the UI thread.
class CONTENT_EXPORT ForwardingAudioStreamFactory final
    : public WebContentsObserver {
 public:
  ForwardingAudioStreamFactory(
      WebContents* web_contents,
      std::unique_ptr<service_manager::Connector> connector,
      std::unique_ptr<AudioStreamBrokerFactory> factory);

  ~ForwardingAudioStreamFactory() final;

  // Returns the ForwardingAudioStreamFactory which takes care of stream
  // creation for |frame|. Returns null if |frame| is null or if the frame
  // doesn't belong to a WebContents.
  static ForwardingAudioStreamFactory* ForFrame(RenderFrameHost* frame);

  const base::UnguessableToken& group_id() { return group_id_; }

  // TODO(https://crbug.com/787806): Automatically restore streams on audio
  // service restart.
  void CreateInputStream(
      RenderFrameHost* frame,
      const std::string& device_id,
      const media::AudioParameters& params,
      uint32_t shared_memory_count,
      bool enable_agc,
      mojom::RendererAudioInputStreamFactoryClientPtr renderer_factory_client);

  void AssociateInputAndOutputForAec(
      const base::UnguessableToken& input_stream_id,
      const std::string& raw_output_device_id);

  void CreateOutputStream(
      RenderFrameHost* frame,
      const std::string& device_id,
      const media::AudioParameters& params,
      media::mojom::AudioOutputStreamProviderClientPtr client);

  void CreateLoopbackStream(
      RenderFrameHost* frame,
      RenderFrameHost* frame_of_source_web_contents,
      const media::AudioParameters& params,
      uint32_t shared_memory_count,
      bool mute_source,
      mojom::RendererAudioInputStreamFactoryClientPtr renderer_factory_client);

  // Sets the muting state for all output streams created through this factory.
  void SetMuted(bool muted);

  // Returns the current muting state.
  bool IsMuted() const;

  // WebContentsObserver implementation. We observe these events so that we can
  // clean up streams belonging to a frame when that frame is destroyed.
  void FrameDeleted(RenderFrameHost* render_frame_host) final;

  // E.g. to override binder.
  service_manager::Connector* get_connector_for_testing() {
    return connector_.get();
  }

 private:
  using StreamBrokerSet = base::flat_set<std::unique_ptr<AudioStreamBroker>,
                                         base::UniquePtrComparator>;

  void CleanupStreamsBelongingTo(RenderFrameHost* render_frame_host);

  void RemoveInput(AudioStreamBroker* handle);
  void RemoveOutput(AudioStreamBroker* handle);

  audio::mojom::StreamFactory* GetFactory();
  void ResetRemoteFactoryPtrIfIdle();
  void ResetRemoteFactoryPtr();

  const std::unique_ptr<service_manager::Connector> connector_;
  const std::unique_ptr<AudioStreamBrokerFactory> broker_factory_;

  // Unique id indentifying all streams belonging to the WebContents owning
  // |this|.
  // TODO(https://crbug.com/824019): Use this for loopback.
  const base::UnguessableToken group_id_;

  // Lazily acquired. Reset on connection error and when we no longer have any
  // streams. Note: we don't want muting to force the connection to be open,
  // since we want to clean up the service when not in use. If we have active
  // muting but nothing else, we should stop it and start it again when we need
  // to reacquire the factory for some other reason.
  audio::mojom::StreamFactoryPtr remote_factory_;

  // Running id used for tracking audible streams. We keep count here to avoid
  // collisions.
  // TODO(https://crbug.com/830494): Refactor to make this unnecessary and
  // remove it.
  int stream_id_counter_ = 0;

  // Instantiated when |outputs_| should be muted, empty otherwise.
  base::Optional<AudioMutingSession> muter_;

  StreamBrokerSet inputs_;
  StreamBrokerSet outputs_;

  DISALLOW_COPY_AND_ASSIGN(ForwardingAudioStreamFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_FORWARDING_AUDIO_STREAM_FACTORY_H_
