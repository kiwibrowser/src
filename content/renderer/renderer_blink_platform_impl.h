// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RENDERER_BLINK_PLATFORM_IMPL_H_
#define CONTENT_RENDERER_RENDERER_BLINK_PLATFORM_IMPL_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "content/child/blink_platform_impl.h"
#include "content/common/content_export.h"
#include "content/common/possibly_associated_interface_ptr.h"
#include "content/renderer/top_level_blame_context.h"
#include "content/renderer/webpublicsuffixlist_impl.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_type.h"
#include "third_party/blink/public/platform/modules/cache_storage/cache_storage.mojom.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_factory.h"
#include "third_party/blink/public/platform/modules/webdatabase/web_database.mojom.h"

namespace IPC {
class SyncMessageFilter;
}

namespace blink {
namespace scheduler {
class WebMainThreadScheduler;
class WebThreadBase;
}
class WebCanvasCaptureHandler;
class WebGraphicsContext3DProvider;
class WebMediaPlayer;
class WebMediaRecorderHandler;
class WebMediaStream;
class WebSecurityOrigin;
}  // namespace blink

namespace device {
class Gamepads;
class MotionData;
class OrientationData;
}

namespace content {
class BlinkInterfaceProviderImpl;
class ChildURLLoaderFactoryBundle;
class LocalStorageCachedAreas;
class PlatformEventObserverBase;
class ThreadSafeSender;
class WebDatabaseObserverImpl;

class CONTENT_EXPORT RendererBlinkPlatformImpl : public BlinkPlatformImpl {
 public:
  explicit RendererBlinkPlatformImpl(
      blink::scheduler::WebMainThreadScheduler* main_thread_scheduler);
  ~RendererBlinkPlatformImpl() override;

  // Shutdown must be called just prior to shutting down blink.
  void Shutdown();

  void set_plugin_refresh_allowed(bool plugin_refresh_allowed) {
    plugin_refresh_allowed_ = plugin_refresh_allowed;
  }
  // Platform methods:
  blink::WebSandboxSupport* GetSandboxSupport() override;
  blink::WebCookieJar* CookieJar() override;
  blink::WebThemeEngine* ThemeEngine() override;
  std::unique_ptr<blink::WebSpeechSynthesizer> CreateSpeechSynthesizer(
      blink::WebSpeechSynthesizerClient* client) override;
  virtual bool sandboxEnabled();
  unsigned long long VisitedLinkHash(const char* canonicalURL,
                                     size_t length) override;
  bool IsLinkVisited(unsigned long long linkHash) override;
  blink::WebPrescientNetworking* PrescientNetworking() override;
  blink::WebString UserAgent() override;
  void CacheMetadata(const blink::WebURL&,
                     base::Time,
                     const char*,
                     size_t) override;
  void CacheMetadataInCacheStorage(
      const blink::WebURL&,
      base::Time,
      const char*,
      size_t,
      const blink::WebSecurityOrigin& cacheStorageOrigin,
      const blink::WebString& cacheStorageCacheName) override;
  blink::WebString DefaultLocale() override;
  void SuddenTerminationChanged(bool enabled) override;
  void AddRefProcess() override;
  void ReleaseRefProcess() override;
  blink::WebThread* CompositorThread() const override;
  std::unique_ptr<blink::WebStorageNamespace> CreateLocalStorageNamespace()
      override;
  std::unique_ptr<blink::WebStorageNamespace> CreateSessionStorageNamespace(
      base::StringPiece namespace_id) override;
  blink::Platform::FileHandle DatabaseOpenFile(
      const blink::WebString& vfs_file_name,
      int desired_flags) override;
  int DatabaseDeleteFile(const blink::WebString& vfs_file_name,
                         bool sync_dir) override;
  long DatabaseGetFileAttributes(
      const blink::WebString& vfs_file_name) override;
  long long DatabaseGetFileSize(const blink::WebString& vfs_file_name) override;
  long long DatabaseGetSpaceAvailableForOrigin(
      const blink::WebSecurityOrigin& origin) override;
  bool DatabaseSetFileSize(const blink::WebString& vfs_file_name,
                           long long size) override;
  blink::WebString DatabaseCreateOriginIdentifier(
      const blink::WebSecurityOrigin& origin) override;
  viz::FrameSinkId GenerateFrameSinkId() override;

  void GetPluginList(bool refresh,
                     const blink::WebSecurityOrigin& mainFrameOrigin,
                     blink::WebPluginListBuilder* builder) override;
  blink::WebPublicSuffixList* PublicSuffixList() override;
  blink::WebScrollbarBehavior* ScrollbarBehavior() override;
  blink::WebIDBFactory* IdbFactory() override;
  blink::WebFileSystem* FileSystem() override;
  blink::WebString FileSystemCreateOriginIdentifier(
      const blink::WebSecurityOrigin& origin) override;

  bool IsThreadedCompositingEnabled() override;
  bool IsThreadedAnimationEnabled() override;
  bool IsGpuCompositingDisabled() override;
  double AudioHardwareSampleRate() override;
  size_t AudioHardwareBufferSize() override;
  unsigned AudioHardwareOutputChannels() override;
  blink::WebDatabaseObserver* DatabaseObserver() override;

  std::unique_ptr<blink::WebAudioDevice> CreateAudioDevice(
      unsigned input_channels,
      unsigned channels,
      const blink::WebAudioLatencyHint& latency_hint,
      blink::WebAudioDevice::RenderCallback* callback,
      const blink::WebString& input_device_id) override;

  bool DecodeAudioFileData(blink::WebAudioBus* destination_bus,
                           const char* audio_file_data,
                           size_t data_size) override;

  std::unique_ptr<blink::WebMIDIAccessor> CreateMIDIAccessor(
      blink::WebMIDIAccessorClient* client) override;

  blink::WebBlobRegistry* GetBlobRegistry() override;
  void SampleGamepads(device::Gamepads&) override;
  std::unique_ptr<blink::WebRTCPeerConnectionHandler>
  CreateRTCPeerConnectionHandler(
      blink::WebRTCPeerConnectionHandlerClient* client,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;
  std::unique_ptr<blink::WebRTCCertificateGenerator>
  CreateRTCCertificateGenerator() override;
  std::unique_ptr<blink::WebMediaRecorderHandler> CreateMediaRecorderHandler(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;
  std::unique_ptr<blink::WebMediaStreamCenter> CreateMediaStreamCenter(
      blink::WebMediaStreamCenterClient* client) override;
  std::unique_ptr<blink::WebCanvasCaptureHandler> CreateCanvasCaptureHandler(
      const blink::WebSize& size,
      double frame_rate,
      blink::WebMediaStreamTrack* track) override;
  void CreateHTMLVideoElementCapturer(
      blink::WebMediaStream* web_media_stream,
      blink::WebMediaPlayer* web_media_player,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;
  void CreateHTMLAudioElementCapturer(
      blink::WebMediaStream* web_media_stream,
      blink::WebMediaPlayer* web_media_player) override;
  std::unique_ptr<blink::WebImageCaptureFrameGrabber>
  CreateImageCaptureFrameGrabber() override;
  void UpdateWebRTCAPICount(blink::WebRTCAPIName api_name) override;
  std::unique_ptr<blink::WebSocketHandshakeThrottle>
  CreateWebSocketHandshakeThrottle() override;
  std::unique_ptr<blink::WebGraphicsContext3DProvider>
  CreateOffscreenGraphicsContext3DProvider(
      const blink::Platform::ContextAttributes& attributes,
      const blink::WebURL& top_document_web_url,
      blink::Platform::GraphicsInfo* gl_info) override;
  std::unique_ptr<blink::WebGraphicsContext3DProvider>
  CreateSharedOffscreenGraphicsContext3DProvider() override;
  gpu::GpuMemoryBufferManager* GetGpuMemoryBufferManager() override;
  blink::WebString ConvertIDNToUnicode(const blink::WebString& host) override;
  service_manager::Connector* GetConnector() override;
  blink::InterfaceProvider* GetInterfaceProvider() override;
  void StartListening(blink::WebPlatformEventType,
                      blink::WebPlatformEventListener*) override;
  void StopListening(blink::WebPlatformEventType) override;
  blink::WebThread* CurrentThread() override;
  blink::BlameContext* GetTopLevelBlameContext() override;
  void RecordRappor(const char* metric,
                    const blink::WebString& sample) override;
  void RecordRapporURL(const char* metric, const blink::WebURL& url) override;
  blink::WebPushProvider* PushProvider() override;
  void DidStartWorkerThread() override;
  void WillStopWorkerThread() override;
  void WorkerContextCreated(const v8::Local<v8::Context>& worker) override;

  // Set the PlatformEventObserverBase in |platform_event_observers_| associated
  // with |type| to |observer|. If there was already an observer associated to
  // the given |type|, it will be replaced.
  // Note that |observer| will be owned by this object after the call.
  void SetPlatformEventObserverForTesting(
      blink::WebPlatformEventType type,
      std::unique_ptr<PlatformEventObserverBase> observer);

  // Disables the WebSandboxSupport implementation for testing.
  // Tests that do not set up a full sandbox environment should call
  // SetSandboxEnabledForTesting(false) _before_ creating any instances
  // of this class, to ensure that we don't attempt to use sandbox-related
  // file descriptors or other resources.
  //
  // Returns the previous |enable| value.
  static bool SetSandboxEnabledForTesting(bool enable);

  // Set MotionData to return when setDeviceMotionListener is invoked.
  static void SetMockDeviceMotionDataForTesting(const device::MotionData& data);
  // Set OrientationData to return when setDeviceOrientationListener
  // is invoked.
  static void SetMockDeviceOrientationDataForTesting(
      const device::OrientationData& data);

  WebDatabaseObserverImpl* web_database_observer_impl() {
    return web_database_observer_impl_.get();
  }

  std::unique_ptr<blink::WebURLLoaderFactory> CreateDefaultURLLoaderFactory()
      override;
  std::unique_ptr<blink::WebURLLoaderFactory> WrapURLLoaderFactory(
      mojo::ScopedMessagePipeHandle url_loader_factory_handle) override;
  std::unique_ptr<blink::WebDataConsumerHandle> CreateDataConsumerHandle(
      mojo::ScopedDataPipeConsumerHandle handle) override;
  void RequestPurgeMemory() override;

  // Returns non-null.
  // It is invalid to call this in an incomplete env where
  // RenderThreadImpl::current() returns nullptr (e.g. in some tests).
  scoped_refptr<ChildURLLoaderFactoryBundle>
  CreateDefaultURLLoaderFactoryBundle();

  // This class does *not* own the compositor thread. It is the responsibility
  // of the caller to ensure that the compositor thread is cleared before it is
  // destructed.
  void SetCompositorThread(blink::scheduler::WebThreadBase* compositor_thread);

  PossiblyAssociatedInterfacePtr<network::mojom::URLLoaderFactory>
  CreateNetworkURLLoaderFactory();

  // Clones the source namespace to the destination namespace.
  void CloneSessionStorageNamespace(const std::string& source_namespace,
                                    const std::string& destination_namespace);

 private:
  bool CheckPreparsedJsCachingEnabled() const;

  // Factory that takes a type and return PlatformEventObserverBase that matches
  // it.
  static std::unique_ptr<PlatformEventObserverBase>
  CreatePlatformEventObserverFromType(blink::WebPlatformEventType type);

  // Use the data previously set via SetMockDevice...DataForTesting() and send
  // them to the registered listener.
  void SendFakeDeviceEventDataForTesting(blink::WebPlatformEventType type);

  // Ensure that the WebDatabaseHost has been initialized.
  void InitializeWebDatabaseHostIfNeeded();

  // Return the mojo interface for making WebDatabaseHost calls.
  blink::mojom::WebDatabaseHost& GetWebDatabaseHost();

  blink::scheduler::WebThreadBase* compositor_thread_;

  std::unique_ptr<blink::WebThread> main_thread_;
  std::unique_ptr<service_manager::Connector> connector_;

#if !defined(OS_ANDROID) && !defined(OS_WIN) && !defined(OS_FUCHSIA)
  class SandboxSupport;
  std::unique_ptr<SandboxSupport> sandbox_support_;
#endif

  // This counter keeps track of the number of times sudden termination is
  // enabled or disabled. It starts at 0 (enabled) and for every disable
  // increments by 1, for every enable decrements by 1. When it reaches 0,
  // we tell the browser to enable fast termination.
  int sudden_termination_disables_;

  // If true, then a GetPlugins call is allowed to rescan the disk.
  bool plugin_refresh_allowed_;

  std::unique_ptr<blink::WebIDBFactory> web_idb_factory_;

  std::unique_ptr<blink::WebBlobRegistry> blob_registry_;

  WebPublicSuffixListImpl public_suffix_list_;

  scoped_refptr<base::SingleThreadTaskRunner> default_task_runner_;
  scoped_refptr<IPC::SyncMessageFilter> sync_message_filter_;
  scoped_refptr<ThreadSafeSender> thread_safe_sender_;

  std::unique_ptr<WebDatabaseObserverImpl> web_database_observer_impl_;

  std::unique_ptr<blink::WebScrollbarBehavior> web_scrollbar_behavior_;

  base::IDMap<std::unique_ptr<PlatformEventObserverBase>>
      platform_event_observers_;

  // NOT OWNED
  blink::scheduler::WebMainThreadScheduler* main_thread_scheduler_;

  TopLevelBlameContext top_level_blame_context_;

  std::unique_ptr<LocalStorageCachedAreas> local_storage_cached_areas_;

  std::unique_ptr<BlinkInterfaceProviderImpl> blink_interface_provider_;

  blink::mojom::WebDatabaseHostPtrInfo web_database_host_info_;
  scoped_refptr<blink::mojom::ThreadSafeWebDatabaseHostPtr> web_database_host_;

  THREAD_CHECKER(main_thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(RendererBlinkPlatformImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_RENDERER_BLINK_PLATFORM_IMPL_H_
