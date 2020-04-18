// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/media_factory.h"

#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial_params.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/buildflag.h"
#include "content/public/common/content_client.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/renderer/media/audio_device_factory.h"
#include "content/renderer/media/render_media_log.h"
#include "content/renderer/media/renderer_webmediaplayer_delegate.h"
#include "content/renderer/media/stream/media_stream_renderer_factory_impl.h"
#include "content/renderer/media/stream/webmediaplayer_ms.h"
#include "content/renderer/media/web_media_element_source_utils.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/render_view_impl.h"
#include "media/base/cdm_factory.h"
#include "media/base/decoder_factory.h"
#include "media/base/media_switches.h"
#include "media/base/renderer_factory_selector.h"
#include "media/base/surface_manager.h"
#include "media/blink/remote_playback_client_wrapper_impl.h"
#include "media/blink/resource_fetch_context.h"
#include "media/blink/webencryptedmediaclient_impl.h"
#include "media/blink/webmediaplayer_impl.h"
#include "media/filters/context_3d.h"
#include "media/media_buildflags.h"
#include "media/renderers/default_decoder_factory.h"
#include "media/renderers/default_renderer_factory.h"
#include "media/video/gpu_video_accelerator_factories.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "services/service_manager/public/cpp/connect.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "third_party/blink/public/platform/web_surface_layer_bridge.h"
#include "third_party/blink/public/platform/web_video_frame_submitter.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/origin.h"

#if defined(OS_ANDROID)
#include "content/renderer/media/android/media_player_renderer_client_factory.h"
#include "content/renderer/media/android/renderer_media_player_manager.h"
#include "content/renderer/media/android/renderer_surface_view_manager.h"
#include "content/renderer/media/android/stream_texture_wrapper_impl.h"
#include "media/base/android/media_codec_util.h"
#include "media/base/media.h"
#include "media/renderers/flinging_renderer_client_factory.h"
#include "url/gurl.h"
#endif

#if BUILDFLAG(ENABLE_MOJO_MEDIA)
#include "content/renderer/media/media_interface_factory.h"
#endif

#if BUILDFLAG(ENABLE_MOJO_CDM)
#include "media/mojo/clients/mojo_cdm_factory.h"  // nogncheck
#endif

#if BUILDFLAG(ENABLE_MOJO_RENDERER)
#include "media/mojo/clients/mojo_renderer_factory.h"  // nogncheck
#endif

#if BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER) || BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)
#include "media/mojo/clients/mojo_decoder_factory.h"  // nogncheck
#endif

#if BUILDFLAG(ENABLE_MEDIA_REMOTING)
#include "media/remoting/courier_renderer_factory.h"    // nogncheck
#include "media/remoting/renderer_controller.h"         // nogncheck
#endif

namespace {
class FrameFetchContext : public media::ResourceFetchContext {
 public:
  explicit FrameFetchContext(blink::WebLocalFrame* frame) : frame_(frame) {
    DCHECK(frame_);
  }
  ~FrameFetchContext() override = default;

  blink::WebLocalFrame* frame() const { return frame_; }

  // media::ResourceFetchContext implementation.
  std::unique_ptr<blink::WebAssociatedURLLoader> CreateUrlLoader(
      const blink::WebAssociatedURLLoaderOptions& options) override {
    return base::WrapUnique(frame_->CreateAssociatedURLLoader(options));
  }

 private:
  blink::WebLocalFrame* frame_;
  DISALLOW_COPY_AND_ASSIGN(FrameFetchContext);
};

void ObtainAndSetContextProvider(
    base::OnceCallback<void(bool,
                            scoped_refptr<ui::ContextProviderCommandBuffer>)>
        set_context_provider_callback,
    std::pair<media::GpuVideoAcceleratorFactories*, bool> gpu_info) {
  if (gpu_info.first) {
    scoped_refptr<ui::ContextProviderCommandBuffer> context_provider =
        gpu_info.first->GetMediaContextProvider();
    std::move(set_context_provider_callback)
        .Run(gpu_info.second, std::move(context_provider));
  } else {
    std::move(set_context_provider_callback).Run(false, nullptr);
  }
}

// Obtains the media ContextProvider and calls the given callback on the same
// thread this is called on. Obtaining the media ContextProvider requires
// getting GPuVideoAcceleratorFactories, which must be done on the main
// thread.
void PostMediaContextProviderToCallback(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    base::OnceCallback<void(bool,
                            scoped_refptr<ui::ContextProviderCommandBuffer>)>
        set_context_provider_callback) {
  base::PostTaskAndReplyWithResult(
      main_task_runner.get(), FROM_HERE, base::BindOnce([]() {
        return std::pair<media::GpuVideoAcceleratorFactories*, bool>(
            content::RenderThreadImpl::current()->GetGpuFactories(),
            !content::RenderThreadImpl::current()->IsGpuCompositingDisabled());
      }),
      base::BindOnce(&ObtainAndSetContextProvider,
                     std::move(set_context_provider_callback)));
}

}  // namespace

namespace content {

MediaFactory::MediaFactory(
    RenderFrameImpl* render_frame,
    media::RequestRoutingTokenCallback request_routing_token_cb)
    : render_frame_(render_frame),
      request_routing_token_cb_(std::move(request_routing_token_cb)) {}

MediaFactory::~MediaFactory() {}

void MediaFactory::SetupMojo() {
  // Only do setup once.
  DCHECK(!remote_interfaces_);

  remote_interfaces_ = render_frame_->GetRemoteInterfaces();
  DCHECK(remote_interfaces_);
}

#if defined(OS_ANDROID)
// Returns true if the MediaPlayerRenderer should be used for playback, false
// if the default renderer should be used instead.
//
// Note that HLS and MP4 detection are pre-redirect and path-based. It is
// possible to load such a URL and find different content.
bool UseMediaPlayerRenderer(const GURL& url) {
  // Always use the default renderer for playing blob URLs.
  if (url.SchemeIsBlob())
    return false;

  // Don't use the default renderer if the container likely contains a codec we
  // can't decode in software and platform decoders are not available.
  if (!media::HasPlatformDecoderSupport()) {
    // Assume that "mp4" means H264. Without platform decoder support we cannot
    // play it with the default renderer so use MediaPlayerRenderer.
    // http://crbug.com/642988.
    if (base::ToLowerASCII(url.spec()).find("mp4") != std::string::npos)
      return true;
  }

  // Indicates if the Android MediaPlayer should be used instead of WMPI.
  if (GetContentClient()->renderer()->ShouldUseMediaPlayerForURL(url))
    return true;

  // Otherwise, use the default renderer.
  return false;
}
#endif  // defined(OS_ANDROID)

blink::WebMediaPlayer* MediaFactory::CreateMediaPlayer(
    const blink::WebMediaPlayerSource& source,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    blink::WebContentDecryptionModule* initial_cdm,
    const blink::WebString& sink_id,
    blink::WebLayerTreeView* layer_tree_view,
    const cc::LayerTreeSettings& settings) {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  blink::WebSecurityOrigin security_origin =
      render_frame_->GetWebFrame()->GetSecurityOrigin();
  blink::WebMediaStream web_stream =
      GetWebMediaStreamFromWebMediaPlayerSource(source);
  if (!web_stream.IsNull())
    return CreateWebMediaPlayerForMediaStream(client, sink_id, security_origin,
                                              web_frame);

  // If |source| was not a MediaStream, it must be a URL.
  // TODO(guidou): Fix this when support for other srcObject types is added.
  DCHECK(source.IsURL());
  blink::WebURL url = source.GetAsURL();

  RenderThreadImpl* render_thread = RenderThreadImpl::current();
  // Render thread may not exist in tests, returning nullptr if it does not.
  if (!render_thread)
    return nullptr;

  scoped_refptr<media::SwitchableAudioRendererSink> audio_renderer_sink =
      AudioDeviceFactory::NewSwitchableAudioRendererSink(
          AudioDeviceFactory::kSourceMediaElement,
          render_frame_->GetRoutingID(), 0, sink_id.Utf8());

  const WebPreferences webkit_preferences =
      render_frame_->GetWebkitPreferences();
  bool embedded_media_experience_enabled = false;
  bool use_media_player_renderer = false;
#if defined(OS_ANDROID)
  use_media_player_renderer = UseMediaPlayerRenderer(url);
  if (!use_media_player_renderer && !media_surface_manager_)
    media_surface_manager_ = new RendererSurfaceViewManager(render_frame_);
  embedded_media_experience_enabled =
      webkit_preferences.embedded_media_experience_enabled;
#endif  // defined(OS_ANDROID)

  // Enable background optimizations based on field trial for src= content, but
  // always enable for MSE content. See http://crbug.com/709302.
  base::TimeDelta max_keyframe_distance_to_disable_background_video =
      base::TimeDelta::FromMilliseconds(base::GetFieldTrialParamByFeatureAsInt(
          media::kBackgroundVideoTrackOptimization, "max_keyframe_distance_ms",
          0));
  base::TimeDelta max_keyframe_distance_to_disable_background_video_mse =
      base::TimeDelta::FromSeconds(5);

  // When memory pressure based garbage collection is enabled for MSE, the
  // |enable_instant_source_buffer_gc| flag controls whether the GC is done
  // immediately on memory pressure notification or during the next SourceBuffer
  // append (slower, but is MSE-spec compliant).
  bool enable_instant_source_buffer_gc =
      base::GetFieldTrialParamByFeatureAsBool(
          media::kMemoryPressureBasedSourceBufferGC,
          "enable_instant_source_buffer_gc", false);

  // This must be created for every new WebMediaPlayer, each instance generates
  // a new player id which is used to collate logs on the browser side.
  std::unique_ptr<media::MediaLog> media_log(new RenderMediaLog(
      url::Origin(security_origin).GetURL(),
      render_frame_->GetTaskRunner(blink::TaskType::kInternalMedia)));

  base::WeakPtr<media::MediaObserver> media_observer;

  auto factory_selector = CreateRendererFactorySelector(
      media_log.get(), use_media_player_renderer, GetDecoderFactory(),
      std::make_unique<media::RemotePlaybackClientWrapperImpl>(client),
      &media_observer);

#if BUILDFLAG(ENABLE_MEDIA_REMOTING)
  DCHECK(media_observer);
#endif

  if (!fetch_context_) {
    fetch_context_ = std::make_unique<FrameFetchContext>(web_frame);
    DCHECK(!url_index_);
    url_index_ = std::make_unique<media::UrlIndex>(fetch_context_.get());
  }
  DCHECK_EQ(static_cast<FrameFetchContext*>(fetch_context_.get())->frame(),
            web_frame);

  media::mojom::MediaMetricsProviderPtr metrics_provider;
  remote_interfaces_->GetInterface(mojo::MakeRequest(&metrics_provider));

  scoped_refptr<base::SingleThreadTaskRunner>
      video_frame_compositor_task_runner;
  std::unique_ptr<blink::WebVideoFrameSubmitter> submitter;
  bool use_surface_layer_for_video =
      base::FeatureList::IsEnabled(media::kUseSurfaceLayerForVideo);
  if (use_surface_layer_for_video) {
    // TODO(lethalantidote): Use a separate task_runner. https://crbug/753605.
    video_frame_compositor_task_runner =
        render_thread->GetMediaThreadTaskRunner();
    submitter = blink::WebVideoFrameSubmitter::Create(
        base::BindRepeating(
            &PostMediaContextProviderToCallback,
            RenderThreadImpl::current()->GetCompositorMainThreadTaskRunner()),
        settings);
  } else {
    video_frame_compositor_task_runner =
        render_thread->compositor_task_runner()
            ? render_thread->compositor_task_runner()
            : render_frame_->GetTaskRunner(
                  blink::TaskType::kInternalMediaRealTime);
  }

  DCHECK(layer_tree_view);
  std::unique_ptr<media::WebMediaPlayerParams> params(
      new media::WebMediaPlayerParams(
          std::move(media_log),
          base::Bind(&ContentRendererClient::DeferMediaLoad,
                     base::Unretained(GetContentClient()->renderer()),
                     static_cast<RenderFrame*>(render_frame_),
                     GetWebMediaPlayerDelegate()->has_played_media()),
          audio_renderer_sink, render_thread->GetMediaThreadTaskRunner(),
          render_thread->GetWorkerTaskRunner(),
          render_thread->compositor_task_runner(),
          video_frame_compositor_task_runner,
          base::Bind(&v8::Isolate::AdjustAmountOfExternalAllocatedMemory,
                     base::Unretained(blink::MainThreadIsolate())),
          initial_cdm, media_surface_manager_, request_routing_token_cb_,
          media_observer, max_keyframe_distance_to_disable_background_video,
          max_keyframe_distance_to_disable_background_video_mse,
          enable_instant_source_buffer_gc, embedded_media_experience_enabled,
          std::move(metrics_provider),
          base::Bind(&blink::WebSurfaceLayerBridge::Create, layer_tree_view),
          RenderThreadImpl::current()->SharedMainThreadContextProvider(),
          use_surface_layer_for_video));

  std::unique_ptr<media::VideoFrameCompositor> vfc =
      std::make_unique<media::VideoFrameCompositor>(
          params->video_frame_compositor_task_runner(), std::move(submitter));

  media::WebMediaPlayerImpl* media_player = new media::WebMediaPlayerImpl(
      web_frame, client, encrypted_client, GetWebMediaPlayerDelegate(),
      std::move(factory_selector), url_index_.get(), std::move(vfc),
      std::move(params));

#if defined(OS_ANDROID)  // WMPI_CAST
  media_player->SetMediaPlayerManager(GetMediaPlayerManager());
  media_player->SetDeviceScaleFactor(
      render_frame_->render_view()->GetDeviceScaleFactor());
#endif  // defined(OS_ANDROID)

  return media_player;
}

blink::WebEncryptedMediaClient* MediaFactory::EncryptedMediaClient() {
  if (!web_encrypted_media_client_) {
    web_encrypted_media_client_.reset(new media::WebEncryptedMediaClientImpl(
        GetCdmFactory(), render_frame_->GetMediaPermission()));
  }
  return web_encrypted_media_client_.get();
}

std::unique_ptr<media::RendererFactorySelector>
MediaFactory::CreateRendererFactorySelector(
    media::MediaLog* media_log,
    bool use_media_player,
    media::DecoderFactory* decoder_factory,
    std::unique_ptr<media::RemotePlaybackClientWrapper> client_wrapper,
    base::WeakPtr<media::MediaObserver>* out_media_observer) {
  RenderThreadImpl* render_thread = RenderThreadImpl::current();
  // Render thread may not exist in tests, returning nullptr if it does not.
  if (!render_thread)
    return nullptr;

  auto factory_selector = std::make_unique<media::RendererFactorySelector>();

#if defined(OS_ANDROID)
  DCHECK(remote_interfaces_);

  // MediaPlayerRendererClientFactory setup.
  auto mojo_media_player_renderer_factory =
      std::make_unique<media::MojoRendererFactory>(
          media::mojom::HostedRendererType::kMediaPlayer,
          media::MojoRendererFactory::GetGpuFactoriesCB(),
          GetMediaInterfaceFactory());

  // Always give |factory_selector| a MediaPlayerRendererClient factory. WMPI
  // might fallback to it if the final redirected URL is an HLS url.
  factory_selector->AddFactory(
      media::RendererFactorySelector::FactoryType::MEDIA_PLAYER,
      std::make_unique<MediaPlayerRendererClientFactory>(
          render_thread->compositor_task_runner(),
          std::move(mojo_media_player_renderer_factory),
          base::Bind(&StreamTextureWrapperImpl::Create,
                     render_thread->EnableStreamTextureCopy(),
                     render_thread->GetStreamTexureFactory(),
                     base::ThreadTaskRunnerHandle::Get())));

  factory_selector->SetUseMediaPlayer(use_media_player);

  // FlingingRendererClientFactory (FRCF) setup.
  auto mojo_flinging_factory = std::make_unique<media::MojoRendererFactory>(
      media::mojom::HostedRendererType::kFlinging,
      media::MojoRendererFactory::GetGpuFactoriesCB(),
      GetMediaInterfaceFactory());

  // Save a temp copy of the pointer, before moving it into the FRCF.
  // The FRCF cannot be aware of the MojoRendererFactory directly, due to
  // layering issues.
  media::MojoRendererFactory* temp_mojo_flinging_factory =
      mojo_flinging_factory.get();

  auto flinging_factory =
      std::make_unique<media::FlingingRendererClientFactory>(
          std::move(mojo_flinging_factory), std::move(client_wrapper));

  // base::Unretained is safe here because the FRCF owns the MojoRendererFactory
  // and is guaranteed to outlive it.
  temp_mojo_flinging_factory->SetGetTypeSpecificIdCB(base::BindRepeating(
      &media::FlingingRendererClientFactory::GetActivePresentationId,
      base::Unretained(flinging_factory.get())));

  // base::Unretained is safe here because |factory_selector| owns
  // |flinging_factory|.
  factory_selector->SetQueryIsFlingingActiveCB(
      base::Bind(&media::FlingingRendererClientFactory::IsFlingingActive,
                 base::Unretained(flinging_factory.get())));

  factory_selector->AddFactory(
      media::RendererFactorySelector::FactoryType::FLINGING,
      std::move(flinging_factory));
#endif  // defined(OS_ANDROID)

  bool use_mojo_renderer_factory = false;
#if BUILDFLAG(ENABLE_MOJO_RENDERER)
#if BUILDFLAG(ENABLE_RUNTIME_MEDIA_RENDERER_SELECTION)
  use_mojo_renderer_factory =
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableMojoRenderer);
#else
  use_mojo_renderer_factory = true;
#endif  // BUILDFLAG(ENABLE_RUNTIME_MEDIA_RENDERER_SELECTION)
  if (use_mojo_renderer_factory) {
    factory_selector->AddFactory(
        media::RendererFactorySelector::FactoryType::MOJO,
        std::make_unique<media::MojoRendererFactory>(
            media::mojom::HostedRendererType::kDefault,
            base::Bind(&RenderThreadImpl::GetGpuFactories,
                       base::Unretained(render_thread)),
            GetMediaInterfaceFactory()));

    factory_selector->SetBaseFactoryType(
        media::RendererFactorySelector::FactoryType::MOJO);
  }
#endif  // BUILDFLAG(ENABLE_MOJO_RENDERER)

  if (!use_mojo_renderer_factory) {
    factory_selector->AddFactory(
        media::RendererFactorySelector::FactoryType::DEFAULT,
        std::make_unique<media::DefaultRendererFactory>(
            media_log, decoder_factory,
            base::Bind(&RenderThreadImpl::GetGpuFactories,
                       base::Unretained(render_thread))));

    factory_selector->SetBaseFactoryType(
        media::RendererFactorySelector::FactoryType::DEFAULT);
  }

#if BUILDFLAG(ENABLE_MEDIA_REMOTING)
  media::mojom::RemotingSourcePtr remoting_source;
  auto remoting_source_request = mojo::MakeRequest(&remoting_source);
  media::mojom::RemoterPtr remoter;
  GetRemoterFactory()->Create(std::move(remoting_source),
                              mojo::MakeRequest(&remoter));
  using RemotingController = media::remoting::RendererController;
  auto remoting_controller = std::make_unique<RemotingController>(
      std::move(remoting_source_request), std::move(remoter));
  *out_media_observer = remoting_controller->GetWeakPtr();

  auto courier_factory =
      std::make_unique<media::remoting::CourierRendererFactory>(
          std::move(remoting_controller));

  // base::Unretained is safe here because |factory_selector| owns
  // |courier_factory|.
  factory_selector->SetQueryIsRemotingActiveCB(
      base::Bind(&media::remoting::CourierRendererFactory::IsRemotingActive,
                 base::Unretained(courier_factory.get())));

  factory_selector->AddFactory(
      media::RendererFactorySelector::FactoryType::COURIER,
      std::move(courier_factory));
#endif

  return factory_selector;
}

blink::WebMediaPlayer* MediaFactory::CreateWebMediaPlayerForMediaStream(
    blink::WebMediaPlayerClient* client,
    const blink::WebString& sink_id,
    const blink::WebSecurityOrigin& security_origin,
    blink::WebLocalFrame* frame) {
  RenderThreadImpl* const render_thread = RenderThreadImpl::current();

  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner =
      render_thread->compositor_task_runner();
  if (!compositor_task_runner.get())
    compositor_task_runner =
        render_frame_->GetTaskRunner(blink::TaskType::kInternalMediaRealTime);

  return new WebMediaPlayerMS(
      frame, client, GetWebMediaPlayerDelegate(),
      std::make_unique<RenderMediaLog>(
          url::Origin(security_origin).GetURL(),
          render_frame_->GetTaskRunner(blink::TaskType::kInternalMedia)),
      CreateMediaStreamRendererFactory(), render_thread->GetIOTaskRunner(),
      compositor_task_runner, render_thread->GetMediaThreadTaskRunner(),
      render_thread->GetWorkerTaskRunner(), render_thread->GetGpuFactories(),
      sink_id);
}

media::RendererWebMediaPlayerDelegate*
MediaFactory::GetWebMediaPlayerDelegate() {
  if (!media_player_delegate_) {
    media_player_delegate_ =
        new media::RendererWebMediaPlayerDelegate(render_frame_);
  }
  return media_player_delegate_;
}

std::unique_ptr<MediaStreamRendererFactory>
MediaFactory::CreateMediaStreamRendererFactory() {
  std::unique_ptr<MediaStreamRendererFactory> factory =
      GetContentClient()->renderer()->CreateMediaStreamRendererFactory();
  if (factory.get())
    return factory;
  return std::unique_ptr<MediaStreamRendererFactory>(
      new MediaStreamRendererFactoryImpl());
}

media::DecoderFactory* MediaFactory::GetDecoderFactory() {
  if (!decoder_factory_) {
    std::unique_ptr<media::DecoderFactory> external_decoder_factory;
#if BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER) || BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)
    external_decoder_factory.reset(
        new media::MojoDecoderFactory(GetMediaInterfaceFactory()));
#endif
    decoder_factory_.reset(
        new media::DefaultDecoderFactory(std::move(external_decoder_factory)));
  }

  return decoder_factory_.get();
}

#if defined(OS_ANDROID)
RendererMediaPlayerManager* MediaFactory::GetMediaPlayerManager() {
  if (!media_player_manager_)
    media_player_manager_ = new RendererMediaPlayerManager(render_frame_);
  return media_player_manager_;
}
#endif  // defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_MEDIA_REMOTING)
media::mojom::RemoterFactory* MediaFactory::GetRemoterFactory() {
  if (!remoter_factory_) {
    DCHECK(remote_interfaces_);
    remote_interfaces_->GetInterface(&remoter_factory_);
  }
  return remoter_factory_.get();
}
#endif

media::CdmFactory* MediaFactory::GetCdmFactory() {
  if (cdm_factory_)
    return cdm_factory_.get();

#if BUILDFLAG(ENABLE_MOJO_CDM)
  cdm_factory_.reset(new media::MojoCdmFactory(GetMediaInterfaceFactory()));
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

  return cdm_factory_.get();
}

#if BUILDFLAG(ENABLE_MOJO_MEDIA)
media::mojom::InterfaceFactory* MediaFactory::GetMediaInterfaceFactory() {
  if (!media_interface_factory_) {
    DCHECK(remote_interfaces_);
    media_interface_factory_.reset(
        new MediaInterfaceFactory(remote_interfaces_));
  }

  return media_interface_factory_.get();
}
#endif  // BUILDFLAG(ENABLE_MOJO_MEDIA)

}  // namespace content
