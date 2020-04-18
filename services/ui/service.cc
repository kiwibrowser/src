// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/service.h"

#include <set>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/threading/platform_thread.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "components/discardable_memory/service/discardable_shared_memory_manager.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/catalog/public/cpp/resource_loader.h"
#include "services/catalog/public/mojom/constants.mojom.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/ui/clipboard/clipboard_impl.h"
#include "services/ui/common/image_cursors_set.h"
#include "services/ui/common/switches.h"
#include "services/ui/display/screen_manager.h"
#include "services/ui/ime/ime_driver_bridge.h"
#include "services/ui/ime/ime_registrar_impl.h"
#include "services/ui/ws/accessibility_manager.h"
#include "services/ui/ws/display_binding.h"
#include "services/ui/ws/display_creation_config.h"
#include "services/ui/ws/display_manager.h"
#include "services/ui/ws/event_injector.h"
#include "services/ui/ws/gpu_host.h"
#include "services/ui/ws/threaded_image_cursors.h"
#include "services/ui/ws/threaded_image_cursors_factory.h"
#include "services/ui/ws/user_activity_monitor.h"
#include "services/ui/ws/user_display_manager.h"
#include "services/ui/ws/window_server.h"
#include "services/ui/ws/window_server_test_impl.h"
#include "services/ui/ws/window_tree.h"
#include "services/ui/ws/window_tree_binding.h"
#include "services/ui/ws/window_tree_factory.h"
#include "services/ui/ws/window_tree_host_factory.h"
#include "ui/base/cursor/image_cursors.h"
#include "ui/base/platform_window_defaults.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "ui/events/event_switches.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gl/gl_surface.h"

#if defined(USE_X11)
#include "ui/base/x/x11_util.h"  // nogncheck
#include "ui/gfx/x/x11.h"
#include "ui/platform_window/x11/x11_window.h"
#elif defined(USE_OZONE)
#include "services/ui/display/screen_manager_forwarding.h"
#include "ui/events/ozone/layout/keyboard_layout_engine.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/ozone_switches.h"
#endif

#if defined(OS_CHROMEOS)
#include "services/ui/public/cpp/input_devices/input_device_controller.h"
#endif

using mojo::InterfaceRequest;
using ui::mojom::WindowServerTest;
using ui::mojom::WindowTreeHostFactory;

namespace ui {

namespace {

const char kResourceFileStrings[] = "mus_app_resources_strings.pak";
const char kResourceFile100[] = "mus_app_resources_100.pak";
const char kResourceFile200[] = "mus_app_resources_200.pak";

class ThreadedImageCursorsFactoryImpl : public ws::ThreadedImageCursorsFactory {
 public:
  explicit ThreadedImageCursorsFactoryImpl(const Service::InitParams& params) {
    // When running in-process use |resource_runner_| to load cursors.
    if (params.resource_runner) {
      resource_runner_ = params.resource_runner;
      // |params.image_cursors_set_weak_ptr| must be set, but don't DCHECK
      // because it can only be dereferenced on |resource_runner_|.
      image_cursors_set_weak_ptr_ = params.image_cursors_set_weak_ptr;
    }
  }

  ~ThreadedImageCursorsFactoryImpl() override = default;

  // ws::ThreadedImageCursorsFactory:
  std::unique_ptr<ws::ThreadedImageCursors> CreateCursors() override {
    // When running out-of-process lazily initialize the resource runner to the
    // UI service's thread.
    if (!resource_runner_) {
      resource_runner_ = base::ThreadTaskRunnerHandle::Get();
      image_cursors_set_ = std::make_unique<ui::ImageCursorsSet>();
      image_cursors_set_weak_ptr_ = image_cursors_set_->GetWeakPtr();
    }
    return std::make_unique<ws::ThreadedImageCursors>(
        resource_runner_, image_cursors_set_weak_ptr_);
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> resource_runner_;
  base::WeakPtr<ui::ImageCursorsSet> image_cursors_set_weak_ptr_;

  // Used when UI Service doesn't run inside WM's process.
  std::unique_ptr<ui::ImageCursorsSet> image_cursors_set_;

  DISALLOW_COPY_AND_ASSIGN(ThreadedImageCursorsFactoryImpl);
};

}  // namespace

// TODO(sky): this is a pretty typical pattern, make it easier to do.
struct Service::PendingRequest {
  service_manager::BindSourceInfo source_info;
  std::unique_ptr<mojom::WindowTreeFactoryRequest> wtf_request;
  std::unique_ptr<mojom::ScreenProviderRequest> screen_request;
};

Service::InitParams::InitParams() = default;

Service::InitParams::~InitParams() = default;

Service::Service(const InitParams& params)
    : running_standalone_(params.running_standalone),
      threaded_image_cursors_factory_(
          std::make_unique<ThreadedImageCursorsFactoryImpl>(params)),
      test_config_(false),
      ime_registrar_(&ime_driver_),
      discardable_shared_memory_manager_(params.memory_manager),
      should_host_viz_(params.should_host_viz) {
  // UI service must host viz when running in its own process.
  DCHECK(!running_standalone_ || should_host_viz_);
}

Service::~Service() {
  in_destructor_ = true;

  // Destroy |window_server_| first, since it depends on |event_source_|.
  // WindowServer (or more correctly its Displays) may have state that needs to
  // be destroyed before GpuState as well.
  window_server_.reset();

  // Must be destroyed before calling OzonePlatform::Shutdown().
  threaded_image_cursors_factory_.reset();

#if defined(OS_CHROMEOS)
  // InputDeviceController uses ozone.
  input_device_controller_.reset();
#endif

#if defined(USE_OZONE)
  OzonePlatform::Shutdown();
#endif
}

bool Service::InitializeResources(service_manager::Connector* connector) {
  if (!running_standalone_ || ui::ResourceBundle::HasSharedInstance())
    return true;

  std::set<std::string> resource_paths;
  resource_paths.insert(kResourceFileStrings);
  resource_paths.insert(kResourceFile100);
  resource_paths.insert(kResourceFile200);

  catalog::ResourceLoader loader;
  filesystem::mojom::DirectoryPtr directory;
  connector->BindInterface(catalog::mojom::kServiceName, &directory);
  if (!loader.OpenFiles(std::move(directory), resource_paths)) {
    LOG(ERROR) << "Service failed to open resource files.";
    return false;
  }

  ui::RegisterPathProvider();

  // Initialize resource bundle with 1x and 2x cursor bitmaps.
  ui::ResourceBundle::InitSharedInstanceWithPakFileRegion(
      loader.TakeFile(kResourceFileStrings),
      base::MemoryMappedFile::Region::kWholeFile);
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  rb.AddDataPackFromFile(loader.TakeFile(kResourceFile100),
                         ui::SCALE_FACTOR_100P);
  rb.AddDataPackFromFile(loader.TakeFile(kResourceFile200),
                         ui::SCALE_FACTOR_200P);
  return true;
}

void Service::OnStart() {
  base::PlatformThread::SetName("mus");
  TRACE_EVENT0("mus", "Service::Initialize started");

  test_config_ = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kUseTestConfig);
#if defined(USE_X11)
  XInitThreads();
  ui::SetDefaultX11ErrorHandlers();
#endif

  if (test_config_)
    ui::test::EnableTestConfigForPlatformWindows();

  // If resources are unavailable do not complete start-up.
  if (!InitializeResources(context()->connector())) {
    context()->QuitNow();
    return;
  }

#if defined(USE_OZONE)
  // The ozone platform can provide its own event source. So initialize the
  // platform before creating the default event source.
  // Because GL libraries need to be initialized before entering the sandbox,
  // in MUS, |InitializeForUI| will load the GL libraries.
  ui::OzonePlatform::InitParams params;
  if (should_host_viz_) {
    // If mus is hosting viz, then it needs to set up ozone so that it can
    // connect to the gpu service through the connector.
    // Currently mus hosting viz (i.e. mash mode) only runs single-process.
    params.connector = context()->connector();
    params.single_process = true;
    params.using_mojo = true;
  } else {
    params.using_mojo = base::CommandLine::ForCurrentProcess()->HasSwitch(
        ::switches::kEnableDrmMojo);
  }
  ui::OzonePlatform::InitializeForUI(params);

  // Assume a client will change the layout to an appropriate configuration.
  ui::KeyboardLayoutEngineManager::GetKeyboardLayoutEngine()
      ->SetCurrentLayoutByName("us");
#endif

#if defined(OS_CHROMEOS)
  input_device_controller_ = std::make_unique<InputDeviceController>();
  input_device_controller_->AddInterface(&registry_);
#endif

#if !defined(OS_ANDROID)
  event_source_ = ui::PlatformEventSource::CreateDefault();
#endif

  // This needs to happen after DeviceDataManager has been constructed. That
  // happens either during OzonePlatform or PlatformEventSource initialization,
  // so keep this line below both of those.
  input_device_server_.RegisterAsObserver();

  if (!discardable_shared_memory_manager_) {
    owned_discardable_shared_memory_manager_ =
        std::make_unique<discardable_memory::DiscardableSharedMemoryManager>();
    discardable_shared_memory_manager_ =
        owned_discardable_shared_memory_manager_.get();
  }

  window_server_ = std::make_unique<ws::WindowServer>(this, should_host_viz_);
  if (should_host_viz_) {
    std::unique_ptr<ws::GpuHost> gpu_host =
        std::make_unique<ws::DefaultGpuHost>(
            window_server_.get(), context()->connector(),
            discardable_shared_memory_manager_);
    window_server_->SetGpuHost(std::move(gpu_host));

    registry_.AddInterface<mojom::Gpu>(
        base::BindRepeating(&Service::BindGpuRequest, base::Unretained(this)));
#if defined(OS_CHROMEOS)
    registry_.AddInterface<mojom::Arc>(
        base::BindRepeating(&Service::BindArcRequest, base::Unretained(this)));
#endif  // defined(OS_CHROMEOS)
  }
  registry_.AddInterface<mojom::VideoDetector>(base::BindRepeating(
      &Service::BindVideoDetectorRequest, base::Unretained(this)));

  ime_driver_.Init(context()->connector(), test_config_);

  registry_with_source_info_.AddInterface<mojom::AccessibilityManager>(
      base::BindRepeating(&Service::BindAccessibilityManagerRequest,
                          base::Unretained(this)));
  registry_with_source_info_.AddInterface<mojom::Clipboard>(base::BindRepeating(
      &Service::BindClipboardRequest, base::Unretained(this)));
  registry_with_source_info_.AddInterface<mojom::ScreenProvider>(
      base::BindRepeating(&Service::BindScreenProviderRequest,
                          base::Unretained(this)));
  registry_.AddInterface<mojom::IMERegistrar>(base::BindRepeating(
      &Service::BindIMERegistrarRequest, base::Unretained(this)));
  registry_.AddInterface<mojom::IMEDriver>(base::BindRepeating(
      &Service::BindIMEDriverRequest, base::Unretained(this)));
  registry_with_source_info_.AddInterface<mojom::UserActivityMonitor>(
      base::BindRepeating(&Service::BindUserActivityMonitorRequest,
                          base::Unretained(this)));
  registry_with_source_info_.AddInterface<WindowTreeHostFactory>(
      base::BindRepeating(&Service::BindWindowTreeHostFactoryRequest,
                          base::Unretained(this)));
  registry_with_source_info_
      .AddInterface<mojom::WindowManagerWindowTreeFactory>(base::BindRepeating(
          &Service::BindWindowManagerWindowTreeFactoryRequest,
          base::Unretained(this)));
  registry_with_source_info_.AddInterface<mojom::WindowTreeFactory>(
      base::BindRepeating(&Service::BindWindowTreeFactoryRequest,
                          base::Unretained(this)));
  registry_with_source_info_
      .AddInterface<discardable_memory::mojom::DiscardableSharedMemoryManager>(
          base::BindRepeating(
              &Service::BindDiscardableSharedMemoryManagerRequest,
              base::Unretained(this)));
  if (test_config_) {
    registry_.AddInterface<WindowServerTest>(base::BindRepeating(
        &Service::BindWindowServerTestRequest, base::Unretained(this)));
  }
  registry_.AddInterface<mojom::EventInjector>(base::BindRepeating(
      &Service::BindEventInjectorRequest, base::Unretained(this)));

  // On non-Linux platforms there will be no DeviceDataManager instance and no
  // purpose in adding the Mojo interface to connect to.
  if (input_device_server_.IsRegisteredAsObserver()) {
    registry_.AddInterface<mojom::InputDeviceServer>(base::BindRepeating(
        &Service::BindInputDeviceServerRequest, base::Unretained(this)));
  }

#if defined(OS_CHROMEOS)
  registry_.AddInterface<mojom::TouchDeviceServer>(base::BindRepeating(
      &Service::BindTouchDeviceServerRequest, base::Unretained(this)));
#endif  // defined(OS_CHROMEOS)

#if defined(USE_OZONE)
  ui::OzonePlatform::GetInstance()->AddInterfaces(&registry_with_source_info_);
#endif
}

void Service::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  if (!registry_with_source_info_.TryBindInterface(
          interface_name, &interface_pipe, source_info)) {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }
}

void Service::StartDisplayInit() {
  DCHECK(!is_gpu_ready_);  // This should only be called once.
  is_gpu_ready_ = true;
  if (screen_manager_)
    screen_manager_->Init(window_server_->display_manager());
}

void Service::OnFirstDisplayReady() {
  PendingRequests requests;
  requests.swap(pending_requests_);
  for (auto& request : requests) {
    if (request->wtf_request) {
      BindWindowTreeFactoryRequest(std::move(*request->wtf_request),
                                   request->source_info);
    } else {
      BindScreenProviderRequest(std::move(*request->screen_request),
                                request->source_info);
    }
  }
}

void Service::OnNoMoreDisplays() {
  // We may get here from the destructor. Don't try to use RequestQuit() when
  // that happens as ServiceContext DCHECKs in this case.
  if (in_destructor_)
    return;

  DCHECK(context());
  context()->CreateQuitClosure().Run();
}

bool Service::IsTestConfig() const {
  return test_config_;
}

void Service::OnWillCreateTreeForWindowManager(
    bool automatically_create_display_roots) {
  if (window_server_->display_creation_config() !=
      ws::DisplayCreationConfig::UNKNOWN) {
    return;
  }

  DVLOG(3) << "OnWillCreateTreeForWindowManager "
           << automatically_create_display_roots;
  ws::DisplayCreationConfig config = automatically_create_display_roots
                                         ? ws::DisplayCreationConfig::AUTOMATIC
                                         : ws::DisplayCreationConfig::MANUAL;
  window_server_->SetDisplayCreationConfig(config);
  if (window_server_->display_creation_config() ==
      ws::DisplayCreationConfig::MANUAL) {
#if defined(OS_CHROMEOS)
    display::ScreenManagerForwarding::Mode mode =
        running_standalone_
            ? display::ScreenManagerForwarding::Mode::OWN_PROCESS
            : display::ScreenManagerForwarding::Mode::IN_WM_PROCESS;
    screen_manager_ = std::make_unique<display::ScreenManagerForwarding>(mode);
#else
    CHECK(false);
#endif
  } else {
    screen_manager_ = display::ScreenManager::Create();
  }
  screen_manager_->AddInterfaces(&registry_with_source_info_);
  if (is_gpu_ready_)
    screen_manager_->Init(window_server_->display_manager());
}

ws::ThreadedImageCursorsFactory* Service::GetThreadedImageCursorsFactory() {
  return threaded_image_cursors_factory_.get();
}

void Service::BindAccessibilityManagerRequest(
    mojom::AccessibilityManagerRequest request,
    const service_manager::BindSourceInfo& source_info) {
  if (!accessibility_) {
    accessibility_ =
        std::make_unique<ws::AccessibilityManager>(window_server_.get());
  }
  accessibility_->Bind(std::move(request));
}

void Service::BindClipboardRequest(
    mojom::ClipboardRequest request,
    const service_manager::BindSourceInfo& source_info) {
  if (!clipboard_)
    clipboard_ = std::make_unique<clipboard::ClipboardImpl>();
  clipboard_->AddBinding(std::move(request));
}

void Service::BindScreenProviderRequest(
    mojom::ScreenProviderRequest request,
    const service_manager::BindSourceInfo& source_info) {
  // Wait for the DisplayManager to be configured before binding display
  // requests. Otherwise the client sees no displays.
  if (!window_server_->display_manager()->IsReady()) {
    std::unique_ptr<PendingRequest> pending_request(new PendingRequest);
    pending_request->source_info = source_info;
    pending_request->screen_request =
        std::make_unique<mojom::ScreenProviderRequest>(std::move(request));
    pending_requests_.push_back(std::move(pending_request));
    return;
  }
  window_server_->display_manager()
      ->GetUserDisplayManager()
      ->AddDisplayManagerBinding(std::move(request));
}

void Service::BindGpuRequest(mojom::GpuRequest request) {
  window_server_->gpu_host()->Add(std::move(request));
}

void Service::BindIMERegistrarRequest(mojom::IMERegistrarRequest request) {
  ime_registrar_.AddBinding(std::move(request));
}

void Service::BindIMEDriverRequest(mojom::IMEDriverRequest request) {
  ime_driver_.AddBinding(std::move(request));
}

void Service::BindInputDeviceServerRequest(
    mojom::InputDeviceServerRequest request) {
  input_device_server_.AddBinding(std::move(request));
}

void Service::BindUserActivityMonitorRequest(
    mojom::UserActivityMonitorRequest request,
    const service_manager::BindSourceInfo& source_info) {
  window_server_->user_activity_monitor()->Add(std::move(request));
}

void Service::BindWindowManagerWindowTreeFactoryRequest(
    mojom::WindowManagerWindowTreeFactoryRequest request,
    const service_manager::BindSourceInfo& source_info) {
  window_server_->BindWindowManagerWindowTreeFactory(std::move(request));
}

void Service::BindWindowTreeFactoryRequest(
    mojom::WindowTreeFactoryRequest request,
    const service_manager::BindSourceInfo& source_info) {
  if (!window_server_->display_manager()->IsReady()) {
    std::unique_ptr<PendingRequest> pending_request(new PendingRequest);
    pending_request->source_info = source_info;
    pending_request->wtf_request.reset(
        new mojom::WindowTreeFactoryRequest(std::move(request)));
    pending_requests_.push_back(std::move(pending_request));
    return;
  }
  mojo::MakeStrongBinding(
      std::make_unique<ws::WindowTreeFactory>(window_server_.get(),
                                              source_info.identity.name()),
      std::move(request));
}

void Service::BindWindowTreeHostFactoryRequest(
    mojom::WindowTreeHostFactoryRequest request,
    const service_manager::BindSourceInfo& source_info) {
  if (!window_tree_host_factory_) {
    window_tree_host_factory_ =
        std::make_unique<ws::WindowTreeHostFactory>(window_server_.get());
  }
  window_tree_host_factory_->AddBinding(std::move(request));
}

void Service::BindDiscardableSharedMemoryManagerRequest(
    discardable_memory::mojom::DiscardableSharedMemoryManagerRequest request,
    const service_manager::BindSourceInfo& source_info) {
  discardable_shared_memory_manager_->Bind(std::move(request), source_info);
}

void Service::BindWindowServerTestRequest(
    mojom::WindowServerTestRequest request) {
  if (!test_config_)
    return;
  mojo::MakeStrongBinding(
      std::make_unique<ws::WindowServerTestImpl>(window_server_.get()),
      std::move(request));
}

void Service::BindEventInjectorRequest(mojom::EventInjectorRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<ws::EventInjector>(window_server_.get()),
      std::move(request));
}

void Service::BindVideoDetectorRequest(mojom::VideoDetectorRequest request) {
  if (!should_host_viz_)
    return;
  window_server_->video_detector()->AddBinding(std::move(request));
}

#if defined(OS_CHROMEOS)
void Service::BindArcRequest(mojom::ArcRequest request) {
  window_server_->gpu_host()->AddArc(std::move(request));
}

void Service::BindTouchDeviceServerRequest(
    mojom::TouchDeviceServerRequest request) {
  touch_device_server_.AddBinding(std::move(request));
}

#endif  // defined(OS_CHROMEOS)

}  // namespace ui
