// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_media_stream_adapter_map.h"

namespace content {

WebRtcMediaStreamAdapterMap::AdapterEntry::AdapterEntry(
    std::unique_ptr<WebRtcMediaStreamAdapter> adapter)
    : adapter(std::move(adapter)), ref_count(0) {
  DCHECK(this->adapter);
}

WebRtcMediaStreamAdapterMap::AdapterEntry::AdapterEntry(AdapterEntry&& other)
    : adapter(other.adapter.release()), ref_count(other.ref_count) {}

WebRtcMediaStreamAdapterMap::AdapterEntry::~AdapterEntry() {
  // |ref_count| is allowed to be non-zero only if this entry has been moved
  // which is the case if the |adapter| has already been released.
  DCHECK(!ref_count || !adapter);
}

WebRtcMediaStreamAdapterMap::AdapterRef::AdapterRef(
    scoped_refptr<WebRtcMediaStreamAdapterMap> map,
    Type type,
    AdapterEntry* adapter_entry)
    : map_(std::move(map)), type_(type), adapter_entry_(adapter_entry) {
  DCHECK(map_);
  DCHECK(adapter_entry_);
  map_->lock_.AssertAcquired();
  ++adapter_entry_->ref_count;
}

WebRtcMediaStreamAdapterMap::AdapterRef::~AdapterRef() {
  DCHECK(map_->main_thread_->BelongsToCurrentThread());
  std::unique_ptr<WebRtcMediaStreamAdapter> removed_adapter;
  {
    base::AutoLock scoped_lock(map_->lock_);
    --adapter_entry_->ref_count;
    if (adapter_entry_->ref_count == 0) {
      removed_adapter = std::move(adapter_entry_->adapter);
      // "GetOrCreate..." ensures the adapter is initialized and the secondary
      // key is set before the last |AdapterRef| is destroyed. We can use either
      // the primary or secondary key for removal.
      if (type_ == Type::kLocal) {
        map_->local_stream_adapters_.EraseByPrimary(
            removed_adapter->web_stream().UniqueId());
      } else {
        map_->remote_stream_adapters_.EraseByPrimary(
            removed_adapter->webrtc_stream().get());
      }
    }
  }
  // Destroy the adapter whilst not holding the lock so that it is safe for
  // destructors to use the signaling thread synchronously without any risk of
  // deadlock.
  removed_adapter.reset();
}

std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>
WebRtcMediaStreamAdapterMap::AdapterRef::Copy() const {
  base::AutoLock scoped_lock(map_->lock_);
  return base::WrapUnique(new AdapterRef(map_, type_, adapter_entry_));
}

WebRtcMediaStreamAdapterMap::WebRtcMediaStreamAdapterMap(
    PeerConnectionDependencyFactory* const factory,
    scoped_refptr<base::SingleThreadTaskRunner> main_thread,
    scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map)
    : factory_(factory),
      main_thread_(std::move(main_thread)),
      track_adapter_map_(std::move(track_adapter_map)) {
  DCHECK(factory_);
  DCHECK(main_thread_);
  DCHECK(track_adapter_map_);
}

WebRtcMediaStreamAdapterMap::~WebRtcMediaStreamAdapterMap() {
  DCHECK(local_stream_adapters_.empty());
  DCHECK(remote_stream_adapters_.empty());
}

std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>
WebRtcMediaStreamAdapterMap::GetLocalStreamAdapter(
    const blink::WebMediaStream& web_stream) {
  base::AutoLock scoped_lock(lock_);
  AdapterEntry* adapter_entry =
      local_stream_adapters_.FindByPrimary(web_stream.UniqueId());
  if (!adapter_entry)
    return nullptr;
  return base::WrapUnique(
      new AdapterRef(this, AdapterRef::Type::kLocal, adapter_entry));
}

std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>
WebRtcMediaStreamAdapterMap::GetLocalStreamAdapter(
    webrtc::MediaStreamInterface* webrtc_stream) {
  base::AutoLock scoped_lock(lock_);
  AdapterEntry* adapter_entry =
      local_stream_adapters_.FindBySecondary(webrtc_stream);
  if (!adapter_entry)
    return nullptr;
  return base::WrapUnique(
      new AdapterRef(this, AdapterRef::Type::kLocal, adapter_entry));
}

std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>
WebRtcMediaStreamAdapterMap::GetOrCreateLocalStreamAdapter(
    const blink::WebMediaStream& web_stream) {
  CHECK(main_thread_->BelongsToCurrentThread());
  CHECK(!web_stream.IsNull());
  base::AutoLock scoped_lock(lock_);
  AdapterEntry* adapter_entry =
      local_stream_adapters_.FindByPrimary(web_stream.UniqueId());
  if (!adapter_entry) {
    std::unique_ptr<WebRtcMediaStreamAdapter> adapter;
    {
      // Make sure we don't hold the lock while calling out to
      // CreateLocalStreamAdapter(). The reason is that constructing a local
      // stream adapter, will synchronize with WebRTC's signaling thread and
      // callbacks on the signaling thread might end up coming back to us
      // and we might need the lock then.
      base::AutoUnlock scoped_unlock(lock_);
      adapter = WebRtcMediaStreamAdapter::CreateLocalStreamAdapter(
          factory_, track_adapter_map_, web_stream);
    }

    adapter_entry = local_stream_adapters_.Insert(web_stream.UniqueId(),
                                                  std::move(adapter));
    CHECK(adapter_entry->adapter->is_initialized());
    CHECK(adapter_entry->adapter->webrtc_stream());
    local_stream_adapters_.SetSecondaryKey(
        web_stream.UniqueId(), adapter_entry->adapter->webrtc_stream().get());
  }
  return base::WrapUnique(
      new AdapterRef(this, AdapterRef::Type::kLocal, adapter_entry));
}

size_t WebRtcMediaStreamAdapterMap::GetLocalStreamCount() const {
  base::AutoLock scoped_lock(lock_);
  return local_stream_adapters_.PrimarySize();
}

std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>
WebRtcMediaStreamAdapterMap::GetRemoteStreamAdapter(
    const blink::WebMediaStream& web_stream) {
  base::AutoLock scoped_lock(lock_);
  AdapterEntry* adapter_entry =
      remote_stream_adapters_.FindBySecondary(web_stream.UniqueId());
  if (!adapter_entry)
    return nullptr;
  return base::WrapUnique(
      new AdapterRef(this, AdapterRef::Type::kRemote, adapter_entry));
}

std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>
WebRtcMediaStreamAdapterMap::GetRemoteStreamAdapter(
    webrtc::MediaStreamInterface* webrtc_stream) {
  base::AutoLock scoped_lock(lock_);
  AdapterEntry* adapter_entry =
      remote_stream_adapters_.FindByPrimary(webrtc_stream);
  if (!adapter_entry)
    return nullptr;
  return base::WrapUnique(
      new AdapterRef(this, AdapterRef::Type::kRemote, adapter_entry));
}

std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>
WebRtcMediaStreamAdapterMap::GetOrCreateRemoteStreamAdapter(
    scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream) {
  CHECK(!main_thread_->BelongsToCurrentThread());
  CHECK(webrtc_stream);
  base::AutoLock scoped_lock(lock_);
  AdapterEntry* adapter_entry =
      remote_stream_adapters_.FindByPrimary(webrtc_stream.get());
  if (!adapter_entry) {
    // Make sure we don't hold the lock while calling out to
    // CreateRemoteStreamAdapter(). The reason is that it might synchronize
    // with other threads, possibly the main thread, where we might need to grab
    // the lock (e.g. inside of GetOrCreateLocalStreamAdapter()).
    std::unique_ptr<WebRtcMediaStreamAdapter> adapter;
    {
      base::AutoUnlock scoped_unlock(lock_);
      adapter = WebRtcMediaStreamAdapter::CreateRemoteStreamAdapter(
          main_thread_, track_adapter_map_, webrtc_stream.get());
    }

    adapter_entry =
        remote_stream_adapters_.Insert(webrtc_stream.get(), std::move(adapter));

    // The new adapter is initialized in a post to the main thread. As soon as
    // it is initialized we map its |webrtc_stream| to the
    // |remote_stream_adapters_| entry as its secondary key. This ensures that
    // there is at least one |AdapterRef| alive until after the adapter is
    // initialized and its secondary key is set.
    main_thread_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &WebRtcMediaStreamAdapterMap::OnRemoteStreamAdapterInitialized,
            this,
            base::WrapUnique(new AdapterRef(this, AdapterRef::Type::kRemote,
                                            adapter_entry))));
  }
  return base::WrapUnique(
      new AdapterRef(this, AdapterRef::Type::kRemote, adapter_entry));
}

size_t WebRtcMediaStreamAdapterMap::GetRemoteStreamCount() const {
  base::AutoLock scoped_lock(lock_);
  return remote_stream_adapters_.PrimarySize();
}

void WebRtcMediaStreamAdapterMap::OnRemoteStreamAdapterInitialized(
    std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> adapter_ref) {
  CHECK(main_thread_->BelongsToCurrentThread());
  CHECK(adapter_ref->is_initialized());
  CHECK(!adapter_ref->adapter().web_stream().IsNull());
  {
    base::AutoLock scoped_lock(lock_);
    remote_stream_adapters_.SetSecondaryKey(
        adapter_ref->adapter().webrtc_stream().get(),
        adapter_ref->adapter().web_stream().UniqueId());
  }
}

}  // namespace content
