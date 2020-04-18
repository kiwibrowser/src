// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_MEDIA_STREAM_ADAPTER_MAP_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_MEDIA_STREAM_ADAPTER_MAP_H_

#include <map>
#include <memory>

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "content/common/content_export.h"
#include "content/renderer/media/webrtc/two_keys_adapter_map.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_adapter.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/webrtc/api/mediastreaminterface.h"

namespace content {

// A map and owner of |WebRtcMediaStreamAdapter|s. Adapters are the glue between
// blink and webrtc layer versions of streams. As long as a stream is in use by
// a peer connection there has to exist an adapter for it. The map takes care of
// creating and disposing stream adapters. Adapters are accessed via
// |AdapterRef|s, when all references to an adapter are destroyed it is
// destroyed and removed from the map.
class CONTENT_EXPORT WebRtcMediaStreamAdapterMap
    : public base::RefCountedThreadSafe<WebRtcMediaStreamAdapterMap> {
 private:
  // The map's entries are reference counted in order to remove the adapter when
  // all |AdapterRef|s referencing an entry are destroyed.
  // Private section needed here due to |AdapterRef|'s usage of |AdapterEntry|.
  struct AdapterEntry {
    AdapterEntry(std::unique_ptr<WebRtcMediaStreamAdapter> adapter);
    AdapterEntry(AdapterEntry&& other);
    ~AdapterEntry();

    AdapterEntry(const AdapterEntry&) = delete;
    AdapterEntry& operator=(const AdapterEntry&) = delete;

    std::unique_ptr<WebRtcMediaStreamAdapter> adapter;
    size_t ref_count;
  };

 public:
  // Accessor to an adapter to take care of reference counting. When the last
  // |AdapterRef| is destroyed, the corresponding adapter is destroyed and
  // removed from the map.
  class CONTENT_EXPORT AdapterRef {
   public:
    // Must be invoked on the main thread. If this was the last reference to the
    // adapter it will be disposed and removed from the map.
    ~AdapterRef();

    std::unique_ptr<AdapterRef> Copy() const;
    bool is_initialized() const { return adapter().is_initialized(); }
    const WebRtcMediaStreamAdapter& adapter() const {
      return *adapter_entry_->adapter;
    }
    WebRtcMediaStreamAdapter& adapter() { return *adapter_entry_->adapter; }

   private:
    friend class WebRtcMediaStreamAdapterMap;

    enum class Type { kLocal, kRemote };

    // Increments the |AdapterEntry::ref_count|. Assumes map's |lock_| is held.
    AdapterRef(scoped_refptr<WebRtcMediaStreamAdapterMap> map,
               Type type,
               AdapterEntry* adapter_entry);

    scoped_refptr<WebRtcMediaStreamAdapterMap> map_;
    Type type_;
    AdapterEntry* adapter_entry_;
  };

  // Must be invoked on the main thread.
  WebRtcMediaStreamAdapterMap(
      PeerConnectionDependencyFactory* const factory,
      scoped_refptr<base::SingleThreadTaskRunner> main_thread,
      scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map);

  // Gets a new reference to the local stream adapter, or null if no such
  // adapter was found. When all references are destroyed the adapter is
  // destroyed and removed from the map. This can be called on any thread, but
  // references must be destroyed on the main thread.
  // The adapter is a associated with a blink and webrtc stream, lookup works by
  // either stream.
  std::unique_ptr<AdapterRef> GetLocalStreamAdapter(
      const blink::WebMediaStream& web_stream);
  std::unique_ptr<AdapterRef> GetLocalStreamAdapter(
      webrtc::MediaStreamInterface* webrtc_stream);
  // Invoke on the main thread. Gets a new reference to the local stream adapter
  // for the web stream. If no adapter exists for the stream one is created.
  // When all references are destroyed the adapter is destroyed and removed from
  // the map. References must be destroyed on the main thread.
  std::unique_ptr<AdapterRef> GetOrCreateLocalStreamAdapter(
      const blink::WebMediaStream& web_stream);
  size_t GetLocalStreamCount() const;

  // Gets a new reference to the remote stream adapter by ID, or null if no such
  // adapter was found. When all references are destroyed the adapter is
  // destroyed and removed from the map. This can be called on any thread, but
  // references must be destroyed on the main thread. The adapter is a
  // associated with a blink and webrtc stream, lookup works by either stream.
  // First variety: If an adapter exists it will already be initialized, if one
  // does not exist null is returned.
  std::unique_ptr<AdapterRef> GetRemoteStreamAdapter(
      const blink::WebMediaStream& web_stream);
  // Second variety: If an adapter exists it may or may not be initialized, see
  // |AdapterRef::is_initialized|. If an adapter does not exist null is
  // returned.
  std::unique_ptr<AdapterRef> GetRemoteStreamAdapter(
      webrtc::MediaStreamInterface* webrtc_stream);
  // Invoke on the webrtc signaling thread. Gets a new reference to the remote
  // stream adapter for the webrtc stream. If no adapter exists for the stream
  // one is created and initialization completes on the main thread in a post.
  // When all references are destroyed the adapter is destroyed and removed from
  // the map. References must be destroyed on the main thread.
  std::unique_ptr<AdapterRef> GetOrCreateRemoteStreamAdapter(
      scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream);
  size_t GetRemoteStreamCount() const;

  const scoped_refptr<WebRtcMediaStreamTrackAdapterMap>& track_adapter_map()
      const {
    return track_adapter_map_;
  }

 private:
  friend class base::RefCountedThreadSafe<WebRtcMediaStreamAdapterMap>;

  // "(blink::WebMediaStream, webrtc::MediaStreamInterface) -> AdapterEntry"
  // maps. The primary key is based on the object used to create the adapter.
  // Local streams are created from |blink::WebMediaStream|s, remote streams are
  // created from |webrtc::MediaStreamInterface|s.
  // The adapter keeps the |webrtc::MediaStreamInterface| alive with ref
  // counting making it safe to use a raw pointer for key.
  using LocalStreamAdapterMap =
      TwoKeysAdapterMap<int,  // blink::WebMediaStream::UniqueId()
                        webrtc::MediaStreamInterface*,
                        AdapterEntry>;
  using RemoteStreamAdapterMap =
      TwoKeysAdapterMap<webrtc::MediaStreamInterface*,
                        int,  // blink::WebMediaStream::UniqueId()
                        AdapterEntry>;

  // Invoke on the main thread.
  virtual ~WebRtcMediaStreamAdapterMap();

  void OnRemoteStreamAdapterInitialized(
      std::unique_ptr<AdapterRef> adapter_ref);

  // Pointer to a |PeerConnectionDependencyFactory| owned by the |RenderThread|.
  // It's valid for the lifetime of |RenderThread|.
  PeerConnectionDependencyFactory* const factory_;
  const scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  // Takes care of creating and owning track adapters, used by stream adapters.
  const scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map_;

  mutable base::Lock lock_;
  LocalStreamAdapterMap local_stream_adapters_;
  RemoteStreamAdapterMap remote_stream_adapters_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_MEDIA_STREAM_ADAPTER_MAP_H_
