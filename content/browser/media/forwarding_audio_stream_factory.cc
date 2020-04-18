// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/forwarding_audio_stream_factory.h"

#include <utility>

#include "base/trace_event/trace_event.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "services/audio/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

ForwardingAudioStreamFactory::ForwardingAudioStreamFactory(
    WebContents* web_contents,
    std::unique_ptr<service_manager::Connector> connector,
    std::unique_ptr<AudioStreamBrokerFactory> broker_factory)
    : WebContentsObserver(web_contents),
      connector_(std::move(connector)),
      broker_factory_(std::move(broker_factory)),
      group_id_(base::UnguessableToken::Create()) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(web_contents);
  DCHECK(broker_factory_);
}

ForwardingAudioStreamFactory::~ForwardingAudioStreamFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

// static
ForwardingAudioStreamFactory* ForwardingAudioStreamFactory::ForFrame(
    RenderFrameHost* frame) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto* contents =
      static_cast<WebContentsImpl*>(WebContents::FromRenderFrameHost(frame));
  if (!contents)
    return nullptr;

  return contents->GetAudioStreamFactory();
}

void ForwardingAudioStreamFactory::CreateInputStream(
    RenderFrameHost* frame,
    const std::string& device_id,
    const media::AudioParameters& params,
    uint32_t shared_memory_count,
    bool enable_agc,
    mojom::RendererAudioInputStreamFactoryClientPtr renderer_factory_client) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const int process_id = frame->GetProcess()->GetID();
  const int frame_id = frame->GetRoutingID();
  inputs_
      .insert(broker_factory_->CreateAudioInputStreamBroker(
          process_id, frame_id, device_id, params, shared_memory_count,
          enable_agc,
          base::BindOnce(&ForwardingAudioStreamFactory::RemoveInput,
                         base::Unretained(this)),
          std::move(renderer_factory_client)))
      .first->get()
      ->CreateStream(GetFactory());
}

void ForwardingAudioStreamFactory::AssociateInputAndOutputForAec(
    const base::UnguessableToken& input_stream_id,
    const std::string& raw_output_device_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Avoid spawning a factory if this for some reason gets called with an
  // invalid input_stream_id before any streams are created.
  if (!inputs_.empty()) {
    GetFactory()->AssociateInputAndOutputForAec(input_stream_id,
                                                raw_output_device_id);
  }
}

void ForwardingAudioStreamFactory::CreateOutputStream(
    RenderFrameHost* frame,
    const std::string& device_id,
    const media::AudioParameters& params,
    media::mojom::AudioOutputStreamProviderClientPtr client) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const int process_id = frame->GetProcess()->GetID();
  const int frame_id = frame->GetRoutingID();

  outputs_
      .insert(broker_factory_->CreateAudioOutputStreamBroker(
          process_id, frame_id, ++stream_id_counter_, device_id, params,
          group_id_,
          base::BindOnce(&ForwardingAudioStreamFactory::RemoveOutput,
                         base::Unretained(this)),
          std::move(client)))
      .first->get()
      ->CreateStream(GetFactory());
}

void ForwardingAudioStreamFactory::CreateLoopbackStream(
    RenderFrameHost* frame,
    RenderFrameHost* frame_of_source_web_contents,
    const media::AudioParameters& params,
    uint32_t shared_memory_count,
    bool mute_source,
    mojom::RendererAudioInputStreamFactoryClientPtr renderer_factory_client) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(frame);
  DCHECK(frame_of_source_web_contents);

  TRACE_EVENT_BEGIN1("audio", "CreateLoopbackStream", "group",
                     group_id_.GetLowForSerialization());

  WebContents* source_contents =
      WebContents::FromRenderFrameHost(frame_of_source_web_contents);
  if (!source_contents) {
    TRACE_EVENT_END1("audio", "CreateLoopbackStream", "source",
                     "failed to find source");
    return;
  }

  const int process_id = frame->GetProcess()->GetID();
  const int frame_id = frame->GetRoutingID();
  inputs_
      .insert(broker_factory_->CreateAudioLoopbackStreamBroker(
          process_id, frame_id,
          std::make_unique<AudioStreamBrokerFactory::LoopbackSource>(
              source_contents),
          params, shared_memory_count, mute_source,
          base::BindOnce(&ForwardingAudioStreamFactory::RemoveInput,
                         base::Unretained(this)),
          std::move(renderer_factory_client)))
      .first->get()
      ->CreateStream(GetFactory());
  TRACE_EVENT_END1("audio", "CreateLoopbackStream", "source",
                   static_cast<WebContentsImpl*>(source_contents)
                       ->GetAudioStreamFactory()
                       ->group_id()
                       .GetLowForSerialization());
}

void ForwardingAudioStreamFactory::SetMuted(bool muted) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_NE(muted, IsMuted());
  TRACE_EVENT_INSTANT2("audio", "SetMuted", TRACE_EVENT_SCOPE_THREAD, "group",
                       group_id_.GetLowForSerialization(), "muted", muted);

  if (!muted) {
    muter_.reset();
    return;
  }

  muter_.emplace(group_id_);
  if (remote_factory_)
    muter_->Connect(remote_factory_.get());
}

bool ForwardingAudioStreamFactory::IsMuted() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return !!muter_;
}

void ForwardingAudioStreamFactory::FrameDeleted(
    RenderFrameHost* render_frame_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  CleanupStreamsBelongingTo(render_frame_host);
}

void ForwardingAudioStreamFactory::CleanupStreamsBelongingTo(
    RenderFrameHost* render_frame_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const int process_id = render_frame_host->GetProcess()->GetID();
  const int frame_id = render_frame_host->GetRoutingID();

  TRACE_EVENT_BEGIN2("audio", "CleanupStreamsBelongingTo", "group",
                     group_id_.GetLowForSerialization(), "process id",
                     process_id);

  auto match_rfh =
      [process_id,
       frame_id](const std::unique_ptr<AudioStreamBroker>& broker) -> bool {
    return broker->render_process_id() == process_id &&
           broker->render_frame_id() == frame_id;
  };

  base::EraseIf(outputs_, match_rfh);
  base::EraseIf(inputs_, match_rfh);

  ResetRemoteFactoryPtrIfIdle();

  TRACE_EVENT_END1("audio", "CleanupStreamsBelongingTo", "frame_id", frame_id);
}

void ForwardingAudioStreamFactory::RemoveInput(AudioStreamBroker* broker) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  size_t removed = inputs_.erase(broker);
  DCHECK_EQ(1u, removed);

  ResetRemoteFactoryPtrIfIdle();
}

void ForwardingAudioStreamFactory::RemoveOutput(AudioStreamBroker* broker) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  size_t removed = outputs_.erase(broker);
  DCHECK_EQ(1u, removed);

  ResetRemoteFactoryPtrIfIdle();
}

audio::mojom::StreamFactory* ForwardingAudioStreamFactory::GetFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!remote_factory_) {
    TRACE_EVENT_INSTANT1(
        "audio", "ForwardingAudioStreamFactory: Binding new factory",
        TRACE_EVENT_SCOPE_THREAD, "group", group_id_.GetLowForSerialization());
    connector_->BindInterface(audio::mojom::kServiceName,
                              mojo::MakeRequest(&remote_factory_));
    // Unretained is safe because |this| owns |remote_factory_|.
    remote_factory_.set_connection_error_handler(
        base::BindOnce(&ForwardingAudioStreamFactory::ResetRemoteFactoryPtr,
                       base::Unretained(this)));

    // Restore the muting session on reconnect.
    if (muter_)
      muter_->Connect(remote_factory_.get());
  }

  return remote_factory_.get();
}

void ForwardingAudioStreamFactory::ResetRemoteFactoryPtrIfIdle() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (inputs_.empty() && outputs_.empty())
    ResetRemoteFactoryPtr();
}

void ForwardingAudioStreamFactory::ResetRemoteFactoryPtr() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (remote_factory_) {
    TRACE_EVENT_INSTANT1(
        "audio", "ForwardingAudioStreamFactory: Resetting factory",
        TRACE_EVENT_SCOPE_THREAD, "group", group_id_.GetLowForSerialization());
  }
  remote_factory_.reset();
  // The stream brokers will call a callback to be deleted soon, give them a
  // chance to signal an error to the client first.
}

}  // namespace content
