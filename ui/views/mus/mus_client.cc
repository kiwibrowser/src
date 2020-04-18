// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/mus_client.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/cpp/gpu/gpu.h"
#include "services/ui/public/cpp/input_devices/input_device_client.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/public/interfaces/event_matcher.mojom.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/capture_synchronizer.h"
#include "ui/aura/mus/mus_context_factory.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/mus/window_tree_host_mus_init_params.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/views/mus/aura_init.h"
#include "ui/views/mus/clipboard_mus.h"
#include "ui/views/mus/desktop_window_tree_host_mus.h"
#include "ui/views/mus/mus_property_mirror.h"
#include "ui/views/mus/pointer_watcher_event_router.h"
#include "ui/views/mus/screen_mus.h"
#include "ui/views/views_delegate.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/wm_state.h"

#if defined(USE_OZONE)
#include "ui/base/cursor/ozone/cursor_data_factory_ozone.h"
#endif

// Widget::InitParams::Type must match that of ui::mojom::WindowType.
#define WINDOW_TYPES_MATCH(NAME)                                      \
  static_assert(                                                      \
      static_cast<int32_t>(views::Widget::InitParams::TYPE_##NAME) == \
          static_cast<int32_t>(ui::mojom::WindowType::NAME),          \
      "Window type constants must match")

WINDOW_TYPES_MATCH(WINDOW);
WINDOW_TYPES_MATCH(PANEL);
WINDOW_TYPES_MATCH(WINDOW_FRAMELESS);
WINDOW_TYPES_MATCH(CONTROL);
WINDOW_TYPES_MATCH(POPUP);
WINDOW_TYPES_MATCH(MENU);
WINDOW_TYPES_MATCH(TOOLTIP);
WINDOW_TYPES_MATCH(BUBBLE);
WINDOW_TYPES_MATCH(DRAG);
// ui::mojom::WindowType::UNKNOWN does not correspond to a value in
// Widget::InitParams::Type.

namespace views {

// static
MusClient* MusClient::instance_ = nullptr;

MusClient::InitParams::InitParams() = default;

MusClient::InitParams::~InitParams() = default;

MusClient::MusClient(const InitParams& params) : identity_(params.identity) {
  DCHECK(!instance_);
  DCHECK(aura::Env::GetInstance());
  instance_ = this;

#if defined(USE_OZONE)
  // If we're in a mus client, we aren't going to have all of ozone initialized
  // even though we're in an ozone build. All the hard coded USE_OZONE ifdefs
  // that handle cursor code expect that there will be a CursorFactoryOzone
  // instance. Partially initialize the ozone cursor internals here, like we
  // partially initialize other ozone subsystems in
  // ChromeBrowserMainExtraPartsViews.
  if (params.create_cursor_factory)
    cursor_factory_ozone_ = std::make_unique<ui::CursorDataFactoryOzone>();
#endif

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner =
      params.io_task_runner;
  if (!io_task_runner) {
    io_thread_ = std::make_unique<base::Thread>("IOThread");
    base::Thread::Options thread_options(base::MessageLoop::TYPE_IO, 0);
    thread_options.priority = base::ThreadPriority::NORMAL;
    CHECK(io_thread_->StartWithOptions(thread_options));
    io_task_runner = io_thread_->task_runner();
  }

  property_converter_ = std::make_unique<aura::PropertyConverter>();
  property_converter_->RegisterPrimitiveProperty(
      ::wm::kShadowElevationKey,
      ui::mojom::WindowManager::kShadowElevation_Property,
      aura::PropertyConverter::CreateAcceptAnyValueCallback());

  if (params.create_wm_state)
    wm_state_ = std::make_unique<wm::WMState>();

  service_manager::Connector* connector = params.connector;
  if (params.bind_test_ws_interfaces) {
    connector->BindInterface(ui::mojom::kServiceName, &server_test_ptr_);
    connector->BindInterface(ui::mojom::kServiceName, &event_injector_);
  }

  if (!params.window_tree_client) {
    DCHECK(io_task_runner);
    owned_window_tree_client_ =
        aura::WindowTreeClient::CreateForWindowTreeFactory(
            connector, this, true, std::move(io_task_runner),
            params.wtc_config);
    window_tree_client_ = owned_window_tree_client_.get();
    aura::Env::GetInstance()->SetWindowTreeClient(window_tree_client_);
  } else {
    window_tree_client_ = params.window_tree_client;
  }

  pointer_watcher_event_router_ =
      std::make_unique<PointerWatcherEventRouter>(window_tree_client_);

  if (connector) {
    input_device_client_ = std::make_unique<ui::InputDeviceClient>();
    ui::mojom::InputDeviceServerPtr input_device_server;
    connector->BindInterface(ui::mojom::kServiceName, &input_device_server);
    input_device_client_->Connect(std::move(input_device_server));

    screen_ = std::make_unique<ScreenMus>(this);
    screen_->Init(connector);

    std::unique_ptr<ClipboardMus> clipboard = std::make_unique<ClipboardMus>();
    clipboard->Init(connector);
    ui::Clipboard::SetClipboardForCurrentThread(std::move(clipboard));
  }

  ViewsDelegate::GetInstance()->set_native_widget_factory(
      base::Bind(&MusClient::CreateNativeWidget, base::Unretained(this)));
  ViewsDelegate::GetInstance()->set_desktop_window_tree_host_factory(base::Bind(
      &MusClient::CreateDesktopWindowTreeHost, base::Unretained(this)));
}

MusClient::~MusClient() {
  // ~WindowTreeClient calls back to us (we're its delegate), destroy it while
  // we are still valid.
  owned_window_tree_client_.reset();
  window_tree_client_ = nullptr;
  ui::OSExchangeDataProviderFactory::SetFactory(nullptr);
  ui::Clipboard::DestroyClipboardForCurrentThread();

  if (ViewsDelegate::GetInstance()) {
    ViewsDelegate::GetInstance()->set_native_widget_factory(
        ViewsDelegate::NativeWidgetFactory());
    ViewsDelegate::GetInstance()->set_desktop_window_tree_host_factory(
        ViewsDelegate::DesktopWindowTreeHostFactory());
  }

  DCHECK_EQ(instance_, this);
  instance_ = nullptr;
  DCHECK(aura::Env::GetInstance());
}

// static
bool MusClient::ShouldCreateDesktopNativeWidgetAura(
    const Widget::InitParams& init_params) {
  // TYPE_CONTROL and child widgets require a NativeWidgetAura.
  return init_params.type != Widget::InitParams::TYPE_CONTROL &&
         !init_params.child;
}

// static
std::map<std::string, std::vector<uint8_t>>
MusClient::ConfigurePropertiesFromParams(
    const Widget::InitParams& init_params) {
  using PrimitiveType = aura::PropertyConverter::PrimitiveType;
  using WindowManager = ui::mojom::WindowManager;
  using TransportType = std::vector<uint8_t>;

  std::map<std::string, TransportType> properties = init_params.mus_properties;

  // Widget::InitParams::Type matches ui::mojom::WindowType.
  properties[WindowManager::kWindowType_InitProperty] =
      mojo::ConvertTo<TransportType>(static_cast<int32_t>(init_params.type));

  properties[WindowManager::kFocusable_InitProperty] =
      mojo::ConvertTo<TransportType>(init_params.CanActivate());

  properties[WindowManager::kTranslucent_InitProperty] =
      mojo::ConvertTo<TransportType>(init_params.opacity ==
                                     Widget::InitParams::TRANSLUCENT_WINDOW);

  if (!init_params.bounds.IsEmpty()) {
    properties[WindowManager::kBounds_InitProperty] =
        mojo::ConvertTo<TransportType>(init_params.bounds);
  }

  if (!init_params.name.empty()) {
    properties[WindowManager::kName_Property] =
        mojo::ConvertTo<TransportType>(init_params.name);
  }

  properties[WindowManager::kAlwaysOnTop_Property] =
      mojo::ConvertTo<TransportType>(
          static_cast<PrimitiveType>(init_params.keep_on_top));

  properties[WindowManager::kRemoveStandardFrame_InitProperty] =
      mojo::ConvertTo<TransportType>(init_params.remove_standard_frame);

  if (init_params.corner_radius) {
    properties[WindowManager::kWindowCornerRadius_Property] =
        mojo::ConvertTo<TransportType>(
            static_cast<PrimitiveType>(*init_params.corner_radius));
  }

  if (!Widget::RequiresNonClientView(init_params.type))
    return properties;

  if (init_params.delegate) {
    if (properties.count(WindowManager::kResizeBehavior_Property) == 0) {
      properties[WindowManager::kResizeBehavior_Property] =
          mojo::ConvertTo<TransportType>(static_cast<PrimitiveType>(
              init_params.delegate->GetResizeBehavior()));
    }

    // TODO(crbug.com/667566): Support additional scales or gfx::Image[Skia].
    gfx::ImageSkia app_icon = init_params.delegate->GetWindowAppIcon();
    SkBitmap app_bitmap = app_icon.GetRepresentation(1.f).sk_bitmap();
    if (!app_bitmap.isNull()) {
      properties[WindowManager::kAppIcon_Property] =
          mojo::ConvertTo<TransportType>(app_bitmap);
    }
    // TODO(crbug.com/667566): Support additional scales or gfx::Image[Skia].
    gfx::ImageSkia window_icon = init_params.delegate->GetWindowIcon();
    SkBitmap window_bitmap = window_icon.GetRepresentation(1.f).sk_bitmap();
    if (!window_bitmap.isNull()) {
      properties[WindowManager::kWindowIcon_Property] =
          mojo::ConvertTo<TransportType>(window_bitmap);
    }
  }

  return properties;
}

NativeWidget* MusClient::CreateNativeWidget(
    const Widget::InitParams& init_params,
    internal::NativeWidgetDelegate* delegate) {
  if (!ShouldCreateDesktopNativeWidgetAura(init_params)) {
    // A null return value results in creating NativeWidgetAura.
    return nullptr;
  }

  DesktopNativeWidgetAura* native_widget =
      new DesktopNativeWidgetAura(delegate);
  if (init_params.desktop_window_tree_host) {
    native_widget->SetDesktopWindowTreeHost(
        base::WrapUnique(init_params.desktop_window_tree_host));
  } else {
    native_widget->SetDesktopWindowTreeHost(
        CreateDesktopWindowTreeHost(init_params, delegate, native_widget));
  }
  return native_widget;
}

void MusClient::OnCaptureClientSet(
    aura::client::CaptureClient* capture_client) {
  pointer_watcher_event_router_->AttachToCaptureClient(capture_client);
  window_tree_client_->capture_synchronizer()->AttachToCaptureClient(
      capture_client);
}

void MusClient::OnCaptureClientUnset(
    aura::client::CaptureClient* capture_client) {
  pointer_watcher_event_router_->DetachFromCaptureClient(capture_client);
  window_tree_client_->capture_synchronizer()->DetachFromCaptureClient(
      capture_client);
}

void MusClient::AddObserver(MusClientObserver* observer) {
  observer_list_.AddObserver(observer);
}

void MusClient::RemoveObserver(MusClientObserver* observer) {
  observer_list_.RemoveObserver(observer);
}
void MusClient::SetMusPropertyMirror(
    std::unique_ptr<MusPropertyMirror> mirror) {
  mus_property_mirror_ = std::move(mirror);
}

void MusClient::CloseAllWidgets() {
  for (aura::Window* root : window_tree_client_->GetRoots()) {
    Widget* widget = Widget::GetWidgetForNativeView(root);
    if (widget)
      widget->CloseNow();
  }
}

ui::mojom::WindowServerTest* MusClient::GetTestingInterface() const {
  // This will only be set in tests. CHECK to ensure it doesn't get used
  // elsewhere.
  CHECK(server_test_ptr_);
  return server_test_ptr_.get();
}

ui::mojom::EventInjector* MusClient::GetTestingEventInjector() const {
  CHECK(event_injector_);
  return event_injector_.get();
}

std::unique_ptr<DesktopWindowTreeHost> MusClient::CreateDesktopWindowTreeHost(
    const Widget::InitParams& init_params,
    internal::NativeWidgetDelegate* delegate,
    DesktopNativeWidgetAura* desktop_native_widget_aura) {
  std::map<std::string, std::vector<uint8_t>> mus_properties =
      ConfigurePropertiesFromParams(init_params);
  aura::WindowTreeHostMusInitParams window_tree_host_init_params =
      aura::CreateInitParamsForTopLevel(MusClient::Get()->window_tree_client(),
                                        std::move(mus_properties));
  return std::make_unique<DesktopWindowTreeHostMus>(
      std::move(window_tree_host_init_params), delegate,
      desktop_native_widget_aura);
}

void MusClient::OnEmbed(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) {
  NOTREACHED();
}

void MusClient::OnLostConnection(aura::WindowTreeClient* client) {}

void MusClient::OnEmbedRootDestroyed(
    aura::WindowTreeHostMus* window_tree_host) {
  static_cast<DesktopWindowTreeHostMus*>(window_tree_host)
      ->ServerDestroyedWindow();
}

void MusClient::OnPointerEventObserved(const ui::PointerEvent& event,
                                       int64_t display_id,
                                       aura::Window* target) {
  pointer_watcher_event_router_->OnPointerEventObserved(event, display_id,
                                                        target);
}

void MusClient::OnWindowManagerFrameValuesChanged() {
  for (auto& observer : observer_list_)
    observer.OnWindowManagerFrameValuesChanged();
}

aura::PropertyConverter* MusClient::GetPropertyConverter() {
  return property_converter_.get();
}

aura::Window* MusClient::GetWindowAtScreenPoint(const gfx::Point& point) {
  for (aura::Window* root : window_tree_client_->GetRoots()) {
    aura::WindowTreeHost* window_tree_host = root->GetHost();
    if (!window_tree_host)
      continue;
    // TODO: this likely gets z-order wrong. http://crbug.com/663606.
    gfx::Point relative_point(point);
    window_tree_host->ConvertScreenInPixelsToDIP(&relative_point);
    if (gfx::Rect(root->bounds().size()).Contains(relative_point))
      return root->GetEventHandlerForPoint(relative_point);
  }
  return nullptr;
}

}  // namespace views
