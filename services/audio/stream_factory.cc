// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/stream_factory.h"

#include <utility>

#include "base/trace_event/trace_event.h"
#include "base/unguessable_token.h"
#include "services/audio/input_stream.h"
#include "services/audio/local_muter.h"
#include "services/audio/loopback_stream.h"
#include "services/audio/output_stream.h"
#include "services/audio/user_input_monitor.h"

namespace audio {

StreamFactory::StreamFactory(media::AudioManager* audio_manager)
    : audio_manager_(audio_manager) {}

StreamFactory::~StreamFactory() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
}

void StreamFactory::Bind(mojom::StreamFactoryRequest request,
                         TracedServiceRef context_ref) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  bindings_.AddBinding(this, std::move(request), std::move(context_ref));
}

void StreamFactory::CreateInputStream(
    media::mojom::AudioInputStreamRequest stream_request,
    media::mojom::AudioInputStreamClientPtr client,
    media::mojom::AudioInputStreamObserverPtr observer,
    media::mojom::AudioLogPtr log,
    const std::string& device_id,
    const media::AudioParameters& params,
    uint32_t shared_memory_count,
    bool enable_agc,
    mojo::ScopedSharedBufferHandle key_press_count_buffer,
    CreateInputStreamCallback created_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT1(
      "audio", "CreateInputStream", bindings_.dispatch_context().id_for_trace(),
      "device id", device_id);

  // Unretained is safe since |this| indirectly owns the InputStream.
  auto deleter_callback = base::BindOnce(&StreamFactory::DestroyInputStream,
                                         base::Unretained(this));

  input_streams_.insert(std::make_unique<InputStream>(
      std::move(created_callback), std::move(deleter_callback),
      std::move(stream_request), std::move(client), std::move(observer),
      std::move(log), audio_manager_,
      UserInputMonitor::Create(std::move(key_press_count_buffer)), device_id,
      params, shared_memory_count, enable_agc));
}

void StreamFactory::AssociateInputAndOutputForAec(
    const base::UnguessableToken& input_stream_id,
    const std::string& output_device_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  for (const auto& stream : input_streams_) {
    if (stream->id() == input_stream_id) {
      stream->SetOutputDeviceForAec(output_device_id);
      return;
    }
  }
}

void StreamFactory::CreateOutputStream(
    media::mojom::AudioOutputStreamRequest stream_request,
    media::mojom::AudioOutputStreamObserverAssociatedPtrInfo observer_info,
    media::mojom::AudioLogPtr log,
    const std::string& output_device_id,
    const media::AudioParameters& params,
    const base::UnguessableToken& group_id,
    CreateOutputStreamCallback created_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT1(
      "audio", "CreateOutputStream",
      bindings_.dispatch_context().id_for_trace(), "device id",
      output_device_id);

  media::mojom::AudioOutputStreamObserverAssociatedPtr observer;
  observer.Bind(std::move(observer_info));

  // Unretained is safe since |this| indirectly owns the OutputStream.
  auto deleter_callback = base::BindOnce(&StreamFactory::DestroyOutputStream,
                                         base::Unretained(this));

  output_streams_.insert(std::make_unique<OutputStream>(
      std::move(created_callback), std::move(deleter_callback),
      std::move(stream_request), std::move(observer), std::move(log),
      audio_manager_, output_device_id, params, &coordinator_, group_id));
}

void StreamFactory::BindMuter(mojom::LocalMuterAssociatedRequest request,
                              const base::UnguessableToken& group_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT1(
      "audio", "BindMuter", bindings_.dispatch_context().id_for_trace(),
      "group id", group_id.GetLowForSerialization());

  // Find the existing LocalMuter for this group, or create one on-demand.
  auto it = std::find_if(muters_.begin(), muters_.end(),
                         [&group_id](const std::unique_ptr<LocalMuter>& muter) {
                           return muter->group_id() == group_id;
                         });
  LocalMuter* muter;
  if (it == muters_.end()) {
    auto muter_ptr = std::make_unique<LocalMuter>(&coordinator_, group_id);
    muter = muter_ptr.get();
    muter->SetAllBindingsLostCallback(base::BindOnce(
        &StreamFactory::DestroyMuter, base::Unretained(this), muter));
    muters_.emplace_back(std::move(muter_ptr));
  } else {
    muter = it->get();
  }

  // Add the binding.
  muter->AddBinding(std::move(request));
}

void StreamFactory::CreateLoopbackStream(
    media::mojom::AudioInputStreamRequest request,
    media::mojom::AudioInputStreamClientPtr client,
    media::mojom::AudioInputStreamObserverPtr observer,
    const media::AudioParameters& params,
    uint32_t shared_memory_count,
    const base::UnguessableToken& group_id,
    CreateLoopbackStreamCallback created_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);

  TRACE_EVENT_NESTABLE_ASYNC_INSTANT1(
      "audio", "CreateLoopbackStream",
      bindings_.dispatch_context().id_for_trace(), "group id",
      group_id.GetLowForSerialization());

  auto stream = std::make_unique<LoopbackStream>(
      std::move(created_callback),
      base::BindOnce(&StreamFactory::DestroyLoopbackStream,
                     base::Unretained(this)),
      audio_manager_->GetWorkerTaskRunner(), std::move(request),
      std::move(client), std::move(observer), params, shared_memory_count,
      &coordinator_, group_id);
  loopback_streams_.emplace_back(std::move(stream));
}

void StreamFactory::DestroyInputStream(InputStream* stream) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  size_t erased = input_streams_.erase(stream);
  DCHECK_EQ(1u, erased);
}

void StreamFactory::DestroyOutputStream(OutputStream* stream) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  size_t erased = output_streams_.erase(stream);
  DCHECK_EQ(1u, erased);
}

void StreamFactory::DestroyMuter(LocalMuter* muter) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  DCHECK(muter);

  const auto it = std::find_if(muters_.begin(), muters_.end(),
                               base::MatchesUniquePtr(muter));
  DCHECK(it != muters_.end());
  muters_.erase(it);
}

void StreamFactory::DestroyLoopbackStream(LoopbackStream* stream) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  DCHECK(stream);

  const auto it =
      std::find_if(loopback_streams_.begin(), loopback_streams_.end(),
                   base::MatchesUniquePtr(stream));
  DCHECK(it != loopback_streams_.end());
  loopback_streams_.erase(it);
}

}  // namespace audio
