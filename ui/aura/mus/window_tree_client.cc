// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/window_tree_client.h"

#include <stddef.h>

#include <string>
#include <utility>
#include <vector>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "cc/base/switches.h"
#include "components/discardable_memory/client/client_discardable_shared_memory_manager.h"
#include "mojo/public/cpp/bindings/map.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/common/accelerator_util.h"
#include "services/ui/common/util.h"
#include "services/ui/public/cpp/gpu/gpu.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "services/ui/public/interfaces/window_manager_window_tree_factory.mojom.h"
#include "services/ui/public/interfaces/window_tree_host_factory.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/drag_drop_client.h"
#include "ui/aura/client/transient_window_client.h"
#include "ui/aura/env.h"
#include "ui/aura/env_input_state_controller.h"
#include "ui/aura/mus/capture_synchronizer.h"
#include "ui/aura/mus/drag_drop_controller_mus.h"
#include "ui/aura/mus/embed_root.h"
#include "ui/aura/mus/embed_root_delegate.h"
#include "ui/aura/mus/focus_synchronizer.h"
#include "ui/aura/mus/in_flight_change.h"
#include "ui/aura/mus/input_method_mus.h"
#include "ui/aura/mus/mus_context_factory.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/property_utils.h"
#include "ui/aura/mus/window_manager_delegate.h"
#include "ui/aura/mus/window_mus.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/mus/window_tree_client_delegate.h"
#include "ui/aura/mus/window_tree_client_observer.h"
#include "ui/aura/mus/window_tree_client_test_observer.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/mus/window_tree_host_mus_init_params.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_port_for_shutdown.h"
#include "ui/aura/window_tracker.h"
#include "ui/base/layout.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches_util.h"
#include "ui/base/ui_base_types.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"

#if defined(USE_OZONE)
#include "ui/aura/mus/platform_event_source_mus_ozone.h"
#endif

namespace aura {
namespace {

struct WindowPortPropertyDataMus : public ui::PropertyData {
  std::string transport_name;
  std::unique_ptr<std::vector<uint8_t>> transport_value;
};

// Handles acknowledgment of an input event, either immediately when a nested
// message loop starts, or upon destruction.
class EventAckHandler : public base::RunLoop::NestingObserver {
 public:
  explicit EventAckHandler(EventResultCallback ack_callback)
      : ack_callback_(std::move(ack_callback)) {
    DCHECK(ack_callback_);
    base::RunLoop::AddNestingObserverOnCurrentThread(this);
  }

  ~EventAckHandler() override {
    base::RunLoop::RemoveNestingObserverOnCurrentThread(this);
    if (ack_callback_) {
      NotifyPlatformEventSource();
      std::move(ack_callback_)
          .Run(handled_ ? ui::mojom::EventResult::HANDLED
                        : ui::mojom::EventResult::UNHANDLED);
    }
  }

#if defined(USE_OZONE)
  void SetPlatformEventSourceAndEvent(
      PlatformEventSourceMus* platform_event_source,
      ui::Event* event) {
    event_ = event;
    platform_event_source_ = platform_event_source;
  }
#endif

  void set_handled(bool handled) { handled_ = handled; }

  // base::RunLoop::NestingObserver:
  void OnBeginNestedRunLoop() override {
    // Acknowledge the event immediately if a nested run loop starts.
    // Otherwise we appear unresponsive for the life of the nested run loop.
    if (ack_callback_) {
      NotifyPlatformEventSource();
      std::move(ack_callback_).Run(ui::mojom::EventResult::HANDLED);
    }
  }

 private:
  void NotifyPlatformEventSource() {
#if defined(USE_OZONE)
    if (platform_event_source_)
      platform_event_source_->OnDidProcessEvent(event_);
#endif
  }

  EventResultCallback ack_callback_;
  bool handled_ = false;
#if defined(USE_OZONE)
  ui::Event* event_ = nullptr;
  PlatformEventSourceMus* platform_event_source_ = nullptr;
#endif

  DISALLOW_COPY_AND_ASSIGN(EventAckHandler);
};

WindowTreeHostMus* GetWindowTreeHostMus(Window* window) {
  return WindowTreeHostMus::ForWindow(window);
}

WindowTreeHostMus* GetWindowTreeHostMus(WindowMus* window) {
  return GetWindowTreeHostMus(window->GetWindow());
}

bool IsInternalProperty(const void* key) {
  return key == client::kModalKey || key == client::kChildModalParentKey;
}

// Create and return a MouseEvent or TouchEvent from |event| if |event| is a
// PointerEvent, otherwise return the copy of |event|.
std::unique_ptr<ui::Event> MapEvent(const ui::Event& event) {
  if (event.IsPointerEvent()) {
    const ui::PointerEvent& pointer_event = *event.AsPointerEvent();
    // Use a switch statement in case more pointer types are added.
    switch (pointer_event.pointer_details().pointer_type) {
      case ui::EventPointerType::POINTER_TYPE_MOUSE:
        if (event.type() == ui::ET_POINTER_WHEEL_CHANGED)
          return std::make_unique<ui::MouseWheelEvent>(pointer_event);
        return std::make_unique<ui::MouseEvent>(pointer_event);
      case ui::EventPointerType::POINTER_TYPE_TOUCH:
      case ui::EventPointerType::POINTER_TYPE_PEN:
        return std::make_unique<ui::TouchEvent>(pointer_event);
      case ui::EventPointerType::POINTER_TYPE_ERASER:
        NOTIMPLEMENTED();
        break;
      case ui::EventPointerType::POINTER_TYPE_UNKNOWN:
        NOTREACHED();
        break;
    }
  }
  return ui::Event::Clone(event);
}

// Use for acks from mus that are expected to always succeed and if they don't
// a crash is triggered.
void OnAckMustSucceed(const base::Location& from_here, bool success) {
  CHECK(success) << "Context: " << from_here.ToString();
}

ui::Id GetServerIdForWindow(Window* window) {
  return window ? WindowMus::Get(window)->server_id() : kInvalidServerId;
}

gfx::Transform ConvertTransformFromServer(WindowMus* window,
                                          const gfx::Transform& transform) {
  const float scale = window->GetDeviceScaleFactor();
  if (scale == 1.0f)
    return transform;

  gfx::Transform dip_transform = transform;
  dip_transform.matrix().set(0, 3, dip_transform.matrix().get(0, 3) / scale);
  dip_transform.matrix().set(1, 3, dip_transform.matrix().get(1, 3) / scale);
  dip_transform.matrix().set(2, 3, dip_transform.matrix().get(2, 3) / scale);
  return dip_transform;
}

// See the comment for ConvertTransformFromServer().
gfx::Transform ConvertTransformToServer(WindowMus* window,
                                        const gfx::Transform& transform) {
  const float scale = window->GetDeviceScaleFactor();
  if (scale == 1.0f)
    return transform;

  gfx::Transform pixel_transform = transform;
  pixel_transform.matrix().set(0, 3, transform.matrix().get(0, 3) * scale);
  pixel_transform.matrix().set(1, 3, transform.matrix().get(1, 3) * scale);
  pixel_transform.matrix().set(2, 3, transform.matrix().get(2, 3) * scale);
  return pixel_transform;
}

}  // namespace

// static
std::unique_ptr<WindowTreeClient> WindowTreeClient::CreateForWindowManager(
    service_manager::Connector* connector,
    WindowTreeClientDelegate* delegate,
    WindowManagerDelegate* window_manager_delegate,
    bool automatically_create_display_roots,
    bool create_discardable_memory) {
  // TODO(sky): this function should be removed when --mash goes away.
  // https://crbug.com/842365.
  std::unique_ptr<WindowTreeClient> wtc(
      new WindowTreeClient(connector, delegate, window_manager_delegate,
                           nullptr, nullptr, create_discardable_memory));

  ui::mojom::WindowManagerWindowTreeFactoryPtr factory;
  connector->BindInterface(ui::mojom::kServiceName, &factory);
  ui::mojom::WindowTreePtr window_tree;
  ui::mojom::WindowTreeClientPtr client;
  wtc->binding_.Bind(MakeRequest(&client));
  factory->CreateWindowTree(MakeRequest(&window_tree), std::move(client),
                            automatically_create_display_roots);
  wtc->SetWindowTree(std::move(window_tree));
  wtc->CreatePlatformEventSourceIfNecessary();
  return wtc;
}

// static
std::unique_ptr<WindowTreeClient> WindowTreeClient::CreateForEmbedding(
    service_manager::Connector* connector,
    WindowTreeClientDelegate* delegate,
    ui::mojom::WindowTreeClientRequest request,
    bool create_discardable_memory) {
  std::unique_ptr<WindowTreeClient> wtc(
      new WindowTreeClient(connector, delegate, nullptr, std::move(request),
                           nullptr, create_discardable_memory));
  return wtc;
}

// static
std::unique_ptr<WindowTreeClient> WindowTreeClient::CreateForWindowTreeFactory(
    service_manager::Connector* connector,
    WindowTreeClientDelegate* delegate,
    bool create_discardable_memory,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    Config config) {
  std::unique_ptr<WindowTreeClient> wtc(
      new WindowTreeClient(connector, delegate, nullptr, nullptr, nullptr,
                           create_discardable_memory, config));
  ui::mojom::WindowTreeFactoryPtr factory;
  connector->BindInterface(ui::mojom::kServiceName, &factory);
  ui::mojom::WindowTreePtr window_tree;
  ui::mojom::WindowTreeClientPtr client;
  wtc->binding_.Bind(MakeRequest(&client));
  factory->CreateWindowTree(MakeRequest(&window_tree), std::move(client));
  wtc->SetWindowTree(std::move(window_tree));
  return wtc;
}

// static
std::unique_ptr<WindowTreeClient>
WindowTreeClient::CreateForWindowTreeHostFactory(
    service_manager::Connector* connector,
    WindowTreeClientDelegate* delegate,
    bool create_discardable_memory) {
  std::unique_ptr<WindowTreeClient> wtc(
      new WindowTreeClient(connector, delegate, nullptr, nullptr, nullptr,
                           create_discardable_memory));
  ui::mojom::WindowTreeHostFactoryPtr factory;
  connector->BindInterface(ui::mojom::kServiceName, &factory);

  ui::mojom::WindowTreeHostPtr window_tree_host;
  ui::mojom::WindowTreeClientPtr client;
  wtc->binding_.Bind(MakeRequest(&client));
  factory->CreateWindowTreeHost(MakeRequest(&window_tree_host),
                                std::move(client));
  return wtc;
}

WindowTreeClient::~WindowTreeClient() {
  in_destructor_ = true;

  if (discardable_shared_memory_manager_)
    base::DiscardableMemoryAllocator::SetInstance(nullptr);

  for (WindowTreeClientObserver& observer : observers_)
    observer.OnWillDestroyClient(this);

  capture_synchronizer_.reset();

  // Some tests may not create a TransientWindowClient.
  if (client::GetTransientWindowClient())
    client::GetTransientWindowClient()->RemoveObserver(this);

  Env* env = Env::GetInstance();
  if (compositor_context_factory_ &&
      env->context_factory() == compositor_context_factory_.get()) {
    env->set_context_factory(initial_context_factory_);
  }

  // Allow for windows to exist (and be created) after we are destroyed. This
  // is necessary because of shutdown ordering (WindowTreeClient is destroyed
  // before windows).
  in_shutdown_ = true;

  // Windows of type WindowMusType::OTHER were implicitly created from the
  // server and may not have been destroyed. Delete them to ensure we don't
  // leak.
  WindowTracker windows_to_destroy;
  for (auto& pair : windows_) {
    if (pair.second->window_mus_type() == WindowMusType::OTHER)
      windows_to_destroy.Add(pair.second->GetWindow());
  }
  while (!windows_to_destroy.windows().empty())
    delete windows_to_destroy.Pop();

  IdToWindowMap windows;
  std::swap(windows, windows_);
  for (auto& pair : windows)
    WindowPortForShutdown::Install(pair.second->GetWindow());

  // EmbedRoots keep a reference to this; so they must all be destroyed before
  // the destructor completes.
  DCHECK(embed_roots_.empty());

  env->WindowTreeClientDestroyed(this);
  CHECK(windows_.empty());
}

void WindowTreeClient::SetCanFocus(Window* window, bool can_focus) {
  DCHECK(tree_);
  DCHECK(window);
  tree_->SetCanFocus(WindowMus::Get(window)->server_id(), can_focus);
}

void WindowTreeClient::SetCursor(WindowMus* window,
                                 const ui::CursorData& old_cursor,
                                 const ui::CursorData& new_cursor) {
  DCHECK(tree_);

  const uint32_t change_id = ScheduleInFlightChange(
      std::make_unique<InFlightCursorChange>(window, old_cursor));
  tree_->SetCursor(change_id, window->server_id(), new_cursor);
}

void WindowTreeClient::SetWindowTextInputState(
    WindowMus* window,
    ui::mojom::TextInputStatePtr state) {
  DCHECK(tree_);
  tree_->SetWindowTextInputState(window->server_id(), std::move(state));
}

void WindowTreeClient::SetImeVisibility(WindowMus* window,
                                        bool visible,
                                        ui::mojom::TextInputStatePtr state) {
  DCHECK(tree_);
  tree_->SetImeVisibility(window->server_id(), visible, std::move(state));
}

void WindowTreeClient::SetHitTestMask(
    WindowMus* window,
    const base::Optional<gfx::Rect>& mask_rect) {
  base::Optional<gfx::Rect> out_rect = base::nullopt;
  if (mask_rect) {
    if (is_using_pixels()) {
      out_rect = gfx::ConvertRectToPixel(window->GetDeviceScaleFactor(),
                                         mask_rect.value());
    } else {
      out_rect = mask_rect.value();
    }
  }

  tree_->SetHitTestMask(window->server_id(), out_rect);
}

void WindowTreeClient::Embed(Window* window,
                             ui::mojom::WindowTreeClientPtr client,
                             uint32_t flags,
                             ui::mojom::WindowTree::EmbedCallback callback) {
  DCHECK(tree_);
  // Window::Init() must be called before Embed() (otherwise the server hasn't
  // been told about the window).
  DCHECK(window->layer());
  if (!window->children().empty()) {
    // The window server removes all children before embedding. In other words,
    // it's generally an error to Embed() with existing children. So, fail
    // early.
    std::move(callback).Run(false);
    return;
  }

  tree_->Embed(WindowMus::Get(window)->server_id(), std::move(client), flags,
               std::move(callback));
}

void WindowTreeClient::ScheduleEmbed(
    ui::mojom::WindowTreeClientPtr client,
    base::OnceCallback<void(const base::UnguessableToken&)> callback) {
  tree_->ScheduleEmbed(std::move(client), std::move(callback));
}

void WindowTreeClient::EmbedUsingToken(
    Window* window,
    const base::UnguessableToken& token,
    uint32_t flags,
    ui::mojom::WindowTree::EmbedCallback callback) {
  DCHECK(tree_);
  // Window::Init() must be called before Embed() (otherwise the server hasn't
  // been told about the window).
  DCHECK(window->layer());
  if (!window->children().empty()) {
    // The window server removes all children before embedding. In other words,
    // it's generally an error to Embed() with existing children. So, fail
    // early.
    std::move(callback).Run(false);
    return;
  }

  tree_->EmbedUsingToken(WindowMus::Get(window)->server_id(), token, flags,
                         std::move(callback));
}

void WindowTreeClient::AttachCompositorFrameSink(
    ui::Id window_id,
    viz::mojom::CompositorFrameSinkRequest compositor_frame_sink,
    viz::mojom::CompositorFrameSinkClientPtr client) {
  DCHECK(tree_);
  tree_->AttachCompositorFrameSink(window_id, std::move(compositor_frame_sink),
                                   std::move(client));
}

std::unique_ptr<EmbedRoot> WindowTreeClient::CreateEmbedRoot(
    EmbedRootDelegate* delegate) {
  std::unique_ptr<EmbedRoot> embed_root =
      base::WrapUnique(new EmbedRoot(this, delegate, next_window_id_++));
  embed_roots_.insert(embed_root.get());
  return embed_root;
}

WindowTreeClient::WindowTreeClient(
    service_manager::Connector* connector,
    WindowTreeClientDelegate* delegate,
    WindowManagerDelegate* window_manager_delegate,
    mojo::InterfaceRequest<ui::mojom::WindowTreeClient> request,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    bool create_discardable_memory,
    Config config)
    : config_(config),
      connector_(connector),
      next_window_id_(1),
      next_change_id_(1),
      delegate_(delegate),
      window_manager_delegate_(window_manager_delegate),
      binding_(this),
      tree_(nullptr),
      in_destructor_(false),
      weak_factory_(this) {
  DCHECK(delegate_);
  // Allow for a null request in tests.
  if (request.is_pending())
    binding_.Bind(std::move(request));
  // Some tests may not create a TransientWindowClient.
  if (client::GetTransientWindowClient())
    client::GetTransientWindowClient()->AddObserver(this);
  if (window_manager_delegate)
    window_manager_delegate->SetWindowManagerClient(this);
  if (connector) {  // |connector| can be null in tests.
    if (!io_task_runner) {
      // |io_task_runner| is null in most case. But for the browser process,
      // the |io_task_runner| is the browser's IO thread.
      io_thread_ = std::make_unique<base::Thread>("IOThread");
      base::Thread::Options thread_options(base::MessageLoop::TYPE_IO, 0);
      thread_options.priority = base::ThreadPriority::NORMAL;
      CHECK(io_thread_->StartWithOptions(thread_options));
      io_task_runner = io_thread_->task_runner();
    }

    gpu_ = ui::Gpu::Create(connector, ui::mojom::kServiceName, io_task_runner);
    compositor_context_factory_ =
        std::make_unique<MusContextFactory>(gpu_.get());
    initial_context_factory_ = Env::GetInstance()->context_factory();
    Env::GetInstance()->set_context_factory(compositor_context_factory_.get());

    // WindowServerTest will create more than one WindowTreeClient. We will not
    // create the discardable memory manager for those tests.
    if (create_discardable_memory) {
      discardable_memory::mojom::DiscardableSharedMemoryManagerPtr manager_ptr;
      connector->BindInterface(ui::mojom::kServiceName, &manager_ptr);
      discardable_shared_memory_manager_ = std::make_unique<
          discardable_memory::ClientDiscardableSharedMemoryManager>(
          std::move(manager_ptr), std::move(io_task_runner));
      base::DiscardableMemoryAllocator::SetInstance(
          discardable_shared_memory_manager_.get());
    }
  }
}

void WindowTreeClient::CreatePlatformEventSourceIfNecessary() {
#if defined(USE_OZONE)
  if (!ui::PlatformEventSource::GetInstance())
    platform_event_source_ = std::make_unique<PlatformEventSourceMus>();
#endif
}

void WindowTreeClient::RegisterWindowMus(WindowMus* window) {
  DCHECK(windows_.find(window->server_id()) == windows_.end());
  windows_[window->server_id()] = window;
  if (window->GetWindow()) {
    auto* port = WindowPortMus::Get(window->GetWindow());
    window->GetWindow()->set_frame_sink_id(
        port->GenerateFrameSinkIdFromServerId());
  }
}

WindowMus* WindowTreeClient::GetWindowByServerId(ui::Id id) {
  IdToWindowMap::const_iterator it = windows_.find(id);
  return it != windows_.end() ? it->second : nullptr;
}

bool WindowTreeClient::IsWindowKnown(aura::Window* window) {
  WindowMus* window_mus = WindowMus::Get(window);
  return windows_.count(window_mus->server_id()) > 0;
}

void WindowTreeClient::ConvertPointerEventLocationToDip(
    int64_t display_id,
    WindowMus* window,
    ui::LocatedEvent* event) const {
  // TODO(sky): this function should be removed when --mash goes away.
  // https://crbug.com/842365.
  if (!is_using_pixels())
    return;

  // PointerEvents shouldn't have the target set.
  DCHECK(!event->target());
  if (window_manager_delegate_) {
    ConvertPointerEventLocationToDipInWindowManager(display_id, window, event);
    return;
  }
  display::Screen* screen = display::Screen::GetScreen();
  display::Display display;
  // TODO(sky): this needs to take into account the ui display scale.
  // http://crbug.com/758399.
  if (!screen->GetDisplayWithDisplayId(display_id, &display) ||
      display.device_scale_factor() == 1.f) {
    return;
  }
  const gfx::Point root_location = gfx::ConvertPointToDIP(
      display.device_scale_factor(), event->root_location());
  event->set_root_location(root_location);
  if (window) {
    const gfx::Point host_location = gfx::ConvertPointToDIP(
        display.device_scale_factor(), event->location());
    event->set_location(host_location);
  } else {
    // When there is no window force the root and location to be the same. They
    // may differ it |window| was valid at the time of the event, but was since
    // deleted.
    event->set_location(root_location);
  }
}

void WindowTreeClient::ConvertPointerEventLocationToDipInWindowManager(
    int64_t display_id,
    WindowMus* window,
    ui::LocatedEvent* event) const {
  // TODO(sky): this function should be removed when --mash goes away.
  // https://crbug.com/842365.
  const WindowTreeHostMus* window_tree_host =
      GetWindowTreeHostForDisplayId(display_id);
  if (!window_tree_host)
    return;

  ui::Event::DispatcherApi dispatcher_api(event);
  if (window) {
    dispatcher_api.set_target(window->GetWindow());
  } else {
    // UpdateForRootTransform() in the case of no target uses |location_|.
    // |location_| may be relative to a window that wasn't found. To ensure we
    // convert from the root, reset |location_| to |root_location_|.
    event->set_location_f(event->root_location_f());
  }
  event->UpdateForRootTransform(
      window_tree_host->GetInverseRootTransform(),
      window_tree_host->GetInverseRootTransformForLocalEventCoordinates());
  dispatcher_api.set_target(nullptr);
}

InFlightChange* WindowTreeClient::GetOldestInFlightChangeMatching(
    const InFlightChange& change) {
  for (const auto& pair : in_flight_map_) {
    if (pair.second->window() == change.window() &&
        pair.second->change_type() == change.change_type() &&
        pair.second->Matches(change)) {
      return pair.second.get();
    }
  }
  return nullptr;
}

uint32_t WindowTreeClient::ScheduleInFlightChange(
    std::unique_ptr<InFlightChange> change) {
  DCHECK(!change->window() ||
         windows_.count(change->window()->server_id()) > 0);
  ChangeType t = change->change_type();
  const uint32_t change_id = next_change_id_++;
  in_flight_map_[change_id] = std::move(change);
  for (auto& observer : test_observers_)
    observer.OnChangeStarted(change_id, t);
  return change_id;
}

bool WindowTreeClient::ApplyServerChangeToExistingInFlightChange(
    const InFlightChange& change) {
  InFlightChange* existing_change = GetOldestInFlightChangeMatching(change);
  if (!existing_change)
    return false;

  existing_change->SetRevertValueFrom(change);
  return true;
}

void WindowTreeClient::BuildWindowTree(
    const std::vector<ui::mojom::WindowDataPtr>& windows) {
  for (const auto& window_data : windows)
    CreateOrUpdateWindowFromWindowData(*window_data);
}

void WindowTreeClient::CreateOrUpdateWindowFromWindowData(
    const ui::mojom::WindowData& window_data) {
  WindowMus* parent = window_data.parent_id == kInvalidServerId
                          ? nullptr
                          : GetWindowByServerId(window_data.parent_id);
  WindowMus* window = GetWindowByServerId(window_data.window_id);
  if (!window)
    window = NewWindowFromWindowData(parent, window_data);
  else if (parent)
    parent->AddChildFromServer(window);

  if (window_data.transient_parent_id == kInvalidServerId)
    return;

  // Some tests may not create a TransientWindowClient.
  client::TransientWindowClient* transient_window_client =
      client::GetTransientWindowClient();
  if (!transient_window_client)
    return;

  // Adjust the transient parent if necessary.
  Window* existing_transient_parent =
      transient_window_client->GetTransientParent(window->GetWindow());
  WindowMus* new_transient_parent =
      GetWindowByServerId(window_data.transient_parent_id);
  if (!new_transient_parent && existing_transient_parent) {
    WindowMus::Get(existing_transient_parent)
        ->RemoveTransientChildFromServer(window);
  } else if (new_transient_parent &&
             new_transient_parent->GetWindow() != existing_transient_parent) {
    if (existing_transient_parent) {
      WindowMus::Get(existing_transient_parent)
          ->RemoveTransientChildFromServer(window);
    }
    new_transient_parent->AddTransientChildFromServer(window);
  }
}

std::unique_ptr<WindowPortMus> WindowTreeClient::CreateWindowPortMus(
    const ui::mojom::WindowData& window_data,
    WindowMusType window_mus_type) {
  std::unique_ptr<WindowPortMus> window_port_mus(
      std::make_unique<WindowPortMus>(this, window_mus_type));
  window_port_mus->set_server_id(window_data.window_id);
  RegisterWindowMus(window_port_mus.get());
  return window_port_mus;
}

void WindowTreeClient::SetLocalPropertiesFromServerProperties(
    WindowMus* window,
    const ui::mojom::WindowData& window_data) {
  for (auto& pair : window_data.properties)
    window->SetPropertyFromServer(pair.first, &pair.second);
}

const WindowTreeHostMus* WindowTreeClient::GetWindowTreeHostForDisplayId(
    int64_t display_id) const {
  if (!window_manager_delegate_)
    return nullptr;

  for (WindowMus* window : roots_) {
    WindowTreeHostMus* window_tree_host =
        static_cast<WindowTreeHostMus*>(window->GetWindow()->GetHost());
    if (window_tree_host->display_id() == display_id)
      return window_tree_host;
  }
  return nullptr;
}

std::unique_ptr<WindowTreeHostMus> WindowTreeClient::CreateWindowTreeHost(
    WindowMusType window_mus_type,
    const ui::mojom::WindowData& window_data,
    int64_t display_id,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  std::unique_ptr<WindowPortMus> window_port =
      CreateWindowPortMus(window_data, window_mus_type);
  roots_.insert(window_port.get());
  WindowTreeHostMusInitParams init_params;
  init_params.window_port = std::move(window_port);
  init_params.window_tree_client = this;
  init_params.display_id = display_id;
  std::unique_ptr<WindowTreeHostMus> window_tree_host =
      std::make_unique<WindowTreeHostMus>(std::move(init_params));
  window_tree_host->InitHost();
  SetLocalPropertiesFromServerProperties(
      WindowMus::Get(window_tree_host->window()), window_data);
  if (window_data.visible) {
    SetWindowVisibleFromServer(WindowMus::Get(window_tree_host->window()),
                               true);
  }
  WindowMus* window = WindowMus::Get(window_tree_host->window());

  SetWindowBoundsFromServer(window, window_data.bounds, local_surface_id);
  return window_tree_host;
}

WindowMus* WindowTreeClient::NewWindowFromWindowData(
    WindowMus* parent,
    const ui::mojom::WindowData& window_data) {
  // This function is only called for windows coming from other clients.
  std::unique_ptr<WindowPortMus> window_port_mus(
      CreateWindowPortMus(window_data, WindowMusType::OTHER));
  WindowPortMus* window_port_mus_ptr = window_port_mus.get();
  // Children of windows created from another client need to be restacked by
  // the client that created them. To do otherwise means two clients will
  // attempt to restack the same windows, leading to raciness (and most likely
  // be rejected by the server anyway).
  window_port_mus_ptr->should_restack_transient_children_ = false;
  Window* window = new Window(nullptr, std::move(window_port_mus));
  WindowMus* window_mus = window_port_mus_ptr;
  std::map<std::string, std::vector<uint8_t>> properties =
      mojo::FlatMapToMap(window_data.properties);
  SetWindowType(window, GetWindowTypeFromProperties(properties));
  window->Init(ui::LAYER_NOT_DRAWN);
  SetLocalPropertiesFromServerProperties(window_mus, window_data);
  window_mus->SetBoundsFromServer(
      is_using_pixels()
          ? gfx::ConvertRectToDIP(window_mus->GetDeviceScaleFactor(),
                                  window_data.bounds)
          : window_data.bounds,
      base::nullopt);
  if (parent)
    parent->AddChildFromServer(window_port_mus_ptr);
  if (window_data.visible)
    window_mus->SetVisibleFromServer(true);
  return window_port_mus_ptr;
}

void WindowTreeClient::SetWindowTree(ui::mojom::WindowTreePtr window_tree_ptr) {
  tree_ptr_ = std::move(window_tree_ptr);

  WindowTreeConnectionEstablished(tree_ptr_.get());
  tree_ptr_->GetCursorLocationMemory(
      base::BindOnce(&WindowTreeClient::OnReceivedCursorLocationMemory,
                     weak_factory_.GetWeakPtr()));

  tree_ptr_.set_connection_error_handler(base::BindOnce(
      &WindowTreeClient::OnConnectionLost, weak_factory_.GetWeakPtr()));

  if (window_manager_delegate_) {
    tree_ptr_->GetWindowManagerClient(
        MakeRequest(&window_manager_internal_client_));
    window_manager_client_ = window_manager_internal_client_.get();
  }
}

void WindowTreeClient::WindowTreeConnectionEstablished(
    ui::mojom::WindowTree* window_tree) {
  tree_ = window_tree;

  drag_drop_controller_ = std::make_unique<DragDropControllerMus>(this, tree_);
  capture_synchronizer_ = std::make_unique<CaptureSynchronizer>(this, tree_);
  focus_synchronizer_ = std::make_unique<FocusSynchronizer>(this, tree_);
}

void WindowTreeClient::OnConnectionLost() {
  delegate_->OnLostConnection(this);
}

bool WindowTreeClient::HandleInternalPropertyChanged(WindowMus* window,
                                                     const void* key,
                                                     int64_t old_value) {
  if (key == client::kModalKey) {
    const uint32_t change_id =
        ScheduleInFlightChange(std::make_unique<InFlightSetModalTypeChange>(
            window, static_cast<ui::ModalType>(old_value)));
    tree_->SetModalType(change_id, window->server_id(),
                        window->GetWindow()->GetProperty(client::kModalKey));
    return true;
  }
  if (key == client::kChildModalParentKey) {
    const uint32_t change_id =
        ScheduleInFlightChange(std::make_unique<CrashInFlightChange>(
            window, ChangeType::CHILD_MODAL_PARENT));
    Window* child_modal_parent =
        window->GetWindow()->GetProperty(client::kChildModalParentKey);
    tree_->SetChildModalParent(
        change_id, window->server_id(),
        child_modal_parent ? WindowMus::Get(child_modal_parent)->server_id()
                           : kInvalidServerId);
    return true;
  }
  return false;
}

void WindowTreeClient::OnEmbedImpl(
    ui::mojom::WindowTree* window_tree,
    ui::mojom::WindowDataPtr root_data,
    int64_t display_id,
    ui::Id focused_window_id,
    bool drawn,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  WindowTreeConnectionEstablished(window_tree);

  DCHECK(roots_.empty());
  std::unique_ptr<WindowTreeHostMus> window_tree_host = CreateWindowTreeHost(
      WindowMusType::EMBED, *root_data, display_id, local_surface_id);

  focus_synchronizer_->SetFocusFromServer(
      GetWindowByServerId(focused_window_id));

  delegate_->OnEmbed(std::move(window_tree_host));
}

EmbedRoot* WindowTreeClient::GetEmbedRootWithRootWindow(aura::Window* window) {
  for (EmbedRoot* embed_root : embed_roots_) {
    if (embed_root->window() == window)
      return embed_root;
  }
  return nullptr;
}

void WindowTreeClient::OnEmbedRootDestroyed(EmbedRoot* embed_root) {
  embed_roots_.erase(embed_root);
}

WindowTreeHostMus* WindowTreeClient::WmNewDisplayAddedImpl(
    const display::Display& display,
    ui::mojom::WindowDataPtr root_data,
    bool parent_drawn,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  // TODO(sky): this function should be removed when --mash goes away.
  // https://crbug.com/842365.
  DCHECK(window_manager_delegate_);

  got_initial_displays_ = true;

  window_manager_delegate_->OnWmWillCreateDisplay(display);

  std::unique_ptr<WindowTreeHostMus> window_tree_host =
      CreateWindowTreeHost(WindowMusType::DISPLAY_AUTOMATICALLY_CREATED,
                           *root_data, display.id(), local_surface_id);

  WindowTreeHostMus* window_tree_host_ptr = window_tree_host.get();
  window_manager_delegate_->OnWmNewDisplay(std::move(window_tree_host),
                                           display);
  return window_tree_host_ptr;
}

EventResultCallback WindowTreeClient::CreateEventResultCallback(
    int32_t event_id) {
  return base::BindOnce(&ui::mojom::WindowTree::OnWindowInputEventAck,
                        base::Unretained(tree_), event_id);
}

void WindowTreeClient::OnReceivedCursorLocationMemory(
    mojo::ScopedSharedBufferHandle handle) {
  cursor_location_mapping_ = handle->Map(sizeof(base::subtle::Atomic32));
  DCHECK(cursor_location_mapping_);
}

void WindowTreeClient::SetWindowBoundsFromServer(
    WindowMus* window,
    const gfx::Rect& revert_bounds,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  if (IsRoot(window)) {
    GetWindowTreeHostMus(window)->SetBoundsFromServerInPixels(
        is_using_pixels() ? revert_bounds
                          : gfx::ConvertRectToPixel(
                                window->GetDeviceScaleFactor(), revert_bounds),
        local_surface_id ? *local_surface_id : viz::LocalSurfaceId());
    return;
  }

  // NOTE: |revert_bounds| is pixels for kMash, and DIPs for kMus2.
  // Window::SetBoundsFromServer() expects DIPs.
  window->SetBoundsFromServer(
      is_using_pixels()
          ? gfx::ConvertRectToDIP(window->GetDeviceScaleFactor(), revert_bounds)
          : revert_bounds,
      local_surface_id);
}

void WindowTreeClient::SetWindowTransformFromServer(
    WindowMus* window,
    const gfx::Transform& transform) {
  if (is_using_pixels()) {
    window->SetTransformFromServer(
        ConvertTransformFromServer(window, transform));
  } else {
    window->SetTransformFromServer(transform);
  }
}

void WindowTreeClient::SetWindowVisibleFromServer(WindowMus* window,
                                                  bool visible) {
  if (!IsRoot(window)) {
    window->SetVisibleFromServer(visible);
    return;
  }

  std::unique_ptr<WindowMusChangeData> data =
      window->PrepareForServerVisibilityChange(visible);
  WindowTreeHostMus* window_tree_host = GetWindowTreeHostMus(window);
  if (visible)
    window_tree_host->Show();
  else
    window_tree_host->Hide();
}

// NOTE: this function takes pixels if is_using_pixels() is true, otherwise
// DIPs. Eventually this will only take DIPs (https://crbug.com/842365).
void WindowTreeClient::ScheduleInFlightBoundsChange(
    WindowMus* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds) {
  const uint32_t change_id =
      ScheduleInFlightChange(std::make_unique<InFlightBoundsChange>(
          this, window, old_bounds, window->GetLocalSurfaceId()));
  base::Optional<viz::LocalSurfaceId> local_surface_id;
  if (window->window_mus_type() == WindowMusType::TOP_LEVEL_IN_WM ||
      window->window_mus_type() == WindowMusType::EMBED_IN_OWNER ||
      window->window_mus_type() == WindowMusType::DISPLAY_MANUALLY_CREATED ||
      window->HasLocalLayerTreeFrameSink()) {
    local_surface_id = window->GetOrAllocateLocalSurfaceId(
        is_using_pixels() ? new_bounds.size()
                          : gfx::ConvertRectToPixel(
                                window->GetDeviceScaleFactor(), new_bounds)
                                .size());
    // |window_tree_host| may be null if this is called during creation of
    // the window associated with the WindowTreeHostMus.
    WindowTreeHost* window_tree_host = window->GetWindow()->GetHost();
    if (window_tree_host)
      window_tree_host->compositor()->OnChildResizing();
  }
  tree_->SetWindowBounds(change_id, window->server_id(), new_bounds,
                         local_surface_id);
}

void WindowTreeClient::OnWindowMusCreated(WindowMus* window) {
  if (window->server_id() != kInvalidServerId)
    return;

  window->set_server_id(next_window_id_++);
  RegisterWindowMus(window);

  DCHECK(window_manager_delegate_ || !IsRoot(window));

  PropertyConverter* property_converter = delegate_->GetPropertyConverter();
  base::flat_map<std::string, std::vector<uint8_t>> transport_properties =
      property_converter->GetTransportProperties(window->GetWindow());

  const uint32_t change_id = ScheduleInFlightChange(
      std::make_unique<CrashInFlightChange>(window, ChangeType::NEW_WINDOW));
  tree_->NewWindow(change_id, window->server_id(),
                   std::move(transport_properties));
  if (window->GetWindow()->event_targeting_policy() !=
      ui::mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS) {
    SetEventTargetingPolicy(window,
                            window->GetWindow()->event_targeting_policy());
  }
  if (window->window_mus_type() == WindowMusType::DISPLAY_MANUALLY_CREATED) {
    WindowTreeHostMus* window_tree_host = GetWindowTreeHostMus(window);
    std::unique_ptr<DisplayInitParams> display_init_params =
        window_tree_host->ReleaseDisplayInitParams();
    DCHECK(display_init_params);
    display::Display display;
    if (display_init_params->display) {
      display = *display_init_params->display;
    } else {
      const bool has_display =
          display::Screen::GetScreen()->GetDisplayWithDisplayId(
              window_tree_host->display_id(), &display);
      DCHECK(has_display);
    }
    // As |window| is a root, changes to its bounds are ignored (it's assumed
    // bounds changes are routed through OnWindowTreeHostBoundsWillChange()).
    // But the display is created with an initial bounds, and we need to push
    // that to the server.
    ScheduleInFlightBoundsChange(
        window, gfx::Rect(),
        gfx::Rect(
            display_init_params->viewport_metrics.bounds_in_pixels.size()));

    if (window_manager_client_) {
      window_manager_client_->SetDisplayRoot(
          display, display_init_params->viewport_metrics.Clone(),
          display_init_params->is_primary_display, window->server_id(),
          display_init_params->mirrors,
          base::BindOnce(&OnAckMustSucceed, FROM_HERE));
    }
  }
}

void WindowTreeClient::OnWindowMusDestroyed(WindowMus* window, Origin origin) {
  if (focus_synchronizer_->focused_window() == window)
    focus_synchronizer_->OnFocusedWindowDestroyed();

  // If we're |in_shutdown_| there is no point in telling the server about the
  // deletion. The connection to the server is about to be dropped and the
  // server will take appropriate action.
  // TODO: decide how to deal with windows not owned by this client.
  if (!in_shutdown_ && origin == Origin::CLIENT &&
      (WasCreatedByThisClient(window) || IsRoot(window))) {
    const uint32_t change_id =
        ScheduleInFlightChange(std::make_unique<CrashInFlightChange>(
            window, ChangeType::DELETE_WINDOW));
    tree_->DeleteWindow(change_id, window->server_id());
  }

  windows_.erase(window->server_id());

  for (auto& entry : embedded_windows_) {
    auto it = entry.second.find(window->GetWindow());
    if (it != entry.second.end()) {
      entry.second.erase(it);
      break;
    }
  }

  // Remove any InFlightChanges associated with the window.
  std::set<uint32_t> in_flight_change_ids_to_remove;
  for (const auto& pair : in_flight_map_) {
    if (pair.second->window() == window)
      in_flight_change_ids_to_remove.insert(pair.first);
  }
  for (auto change_id : in_flight_change_ids_to_remove)
    in_flight_map_.erase(change_id);

  roots_.erase(window);
}

void WindowTreeClient::OnWindowMusBoundsChanged(WindowMus* window,
                                                const gfx::Rect& old_bounds,
                                                const gfx::Rect& new_bounds) {
  // Changes to bounds of root windows are routed through
  // OnWindowTreeHostBoundsWillChange(). Any bounds that happen here are a side
  // effect of those and can be ignored.
  if (IsRoot(window)) {
    // Do not set the LocalSurfaceId on the compositor here, because it has
    // already been set.
    return;
  }
  const float device_scale_factor = window->GetDeviceScaleFactor();
  ScheduleInFlightBoundsChange(
      window,
      is_using_pixels()
          ? gfx::ConvertRectToPixel(device_scale_factor, old_bounds)
          : old_bounds,
      is_using_pixels()
          ? gfx::ConvertRectToPixel(device_scale_factor, new_bounds)
          : new_bounds);
}

void WindowTreeClient::OnWindowMusTransformChanged(
    WindowMus* window,
    const gfx::Transform& old_transform,
    const gfx::Transform& new_transform) {
  const uint32_t change_id = ScheduleInFlightChange(
      std::make_unique<InFlightTransformChange>(this, window, old_transform));
  tree_->SetWindowTransform(
      change_id, window->server_id(),
      is_using_pixels() ? ConvertTransformToServer(window, new_transform)
                        : new_transform);
}

void WindowTreeClient::OnWindowMusAddChild(WindowMus* parent,
                                           WindowMus* child) {
  // TODO: add checks to ensure this can work.
  const uint32_t change_id = ScheduleInFlightChange(
      std::make_unique<CrashInFlightChange>(parent, ChangeType::ADD_CHILD));
  tree_->AddWindow(change_id, parent->server_id(), child->server_id());
}

void WindowTreeClient::OnWindowMusRemoveChild(WindowMus* parent,
                                              WindowMus* child) {
  // TODO: add checks to ensure this can work.
  const uint32_t change_id = ScheduleInFlightChange(
      std::make_unique<CrashInFlightChange>(parent, ChangeType::REMOVE_CHILD));
  tree_->RemoveWindowFromParent(change_id, child->server_id());
}

void WindowTreeClient::OnWindowMusMoveChild(WindowMus* parent,
                                            size_t current_index,
                                            size_t dest_index) {
  DCHECK_NE(current_index, dest_index);
  // TODO: add checks to ensure this can work, e.g. we own the parent.
  const uint32_t change_id = ScheduleInFlightChange(
      std::make_unique<CrashInFlightChange>(parent, ChangeType::REORDER));
  WindowMus* window =
      WindowMus::Get(parent->GetWindow()->children()[current_index]);
  WindowMus* relative_window = nullptr;
  ui::mojom::OrderDirection direction;
  if (dest_index < current_index) {
    relative_window =
        WindowMus::Get(parent->GetWindow()->children()[dest_index]);
    direction = ui::mojom::OrderDirection::BELOW;
  } else {
    relative_window =
        WindowMus::Get(parent->GetWindow()->children()[dest_index]);
    direction = ui::mojom::OrderDirection::ABOVE;
  }
  tree_->ReorderWindow(change_id, window->server_id(),
                       relative_window->server_id(), direction);
}

void WindowTreeClient::OnWindowMusSetVisible(WindowMus* window, bool visible) {
  // TODO: add checks to ensure this can work.
  DCHECK(tree_);
  const uint32_t change_id = ScheduleInFlightChange(
      std::make_unique<InFlightVisibleChange>(this, window, !visible));
  tree_->SetWindowVisibility(change_id, window->server_id(), visible);
}

std::unique_ptr<ui::PropertyData>
WindowTreeClient::OnWindowMusWillChangeProperty(WindowMus* window,
                                                const void* key) {
  if (IsInternalProperty(key))
    return nullptr;

  std::unique_ptr<WindowPortPropertyDataMus> data(
      std::make_unique<WindowPortPropertyDataMus>());
  if (!delegate_->GetPropertyConverter()->ConvertPropertyForTransport(
          window->GetWindow(), key, &data->transport_name,
          &data->transport_value)) {
    return nullptr;
  }
  return std::move(data);
}

void WindowTreeClient::OnWindowMusPropertyChanged(
    WindowMus* window,
    const void* key,
    int64_t old_value,
    std::unique_ptr<ui::PropertyData> data) {
  if (HandleInternalPropertyChanged(window, key, old_value) || !data)
    return;

  WindowPortPropertyDataMus* data_mus =
      static_cast<WindowPortPropertyDataMus*>(data.get());

  std::string transport_name;
  std::unique_ptr<std::vector<uint8_t>> transport_value;
  if (!delegate_->GetPropertyConverter()->ConvertPropertyForTransport(
          window->GetWindow(), key, &transport_name, &transport_value)) {
    return;
  }
  DCHECK_EQ(transport_name, data_mus->transport_name);

  base::Optional<std::vector<uint8_t>> transport_value_mojo;
  if (transport_value)
    transport_value_mojo.emplace(std::move(*transport_value));

  const uint32_t change_id =
      ScheduleInFlightChange(std::make_unique<InFlightPropertyChange>(
          window, transport_name, std::move(data_mus->transport_value)));
  tree_->SetWindowProperty(change_id, window->server_id(), transport_name,
                           transport_value_mojo);
}

void WindowTreeClient::OnWindowMusDeviceScaleFactorChanged(
    WindowMus* window,
    float old_scale_factor,
    float new_scale_factor) {
  if (!is_using_pixels())
    return;  // device-scale factor doesn't impact DIP bounds.

  // Root changes are handled else where.
  if (IsRoot(window))
    return;

  const gfx::Rect old_bounds =
      gfx::ConvertRectToPixel(old_scale_factor, window->GetWindow()->bounds());
  const gfx::Rect new_bounds =
      gfx::ConvertRectToPixel(new_scale_factor, window->GetWindow()->bounds());
  ScheduleInFlightBoundsChange(window, old_bounds, new_bounds);
}

void WindowTreeClient::OnWmMoveLoopCompleted(uint32_t change_id,
                                             bool completed) {
  if (window_manager_client_)
    window_manager_client_->WmResponse(change_id, completed);

  if (change_id == current_wm_move_loop_change_) {
    current_wm_move_loop_change_ = 0;
    current_wm_move_loop_window_id_ = 0;
  }
}

std::set<Window*> WindowTreeClient::GetRoots() {
  std::set<Window*> roots;
  for (WindowMus* window : roots_)
    roots.insert(window->GetWindow());
  return roots;
}

bool WindowTreeClient::WasCreatedByThisClient(const WindowMus* window) const {
  // Windows created via CreateTopLevelWindow() are not owned by us, but don't
  // have high-word set. const_cast is required by set.
  return !ui::ClientIdFromTransportId(window->server_id()) &&
         roots_.count(const_cast<WindowMus*>(window)) == 0;
}

gfx::Point WindowTreeClient::GetCursorScreenPoint() {
  if (!cursor_location_memory())
    return gfx::Point();  // We raced initialization. Return (0, 0).

  base::subtle::Atomic32 location =
      base::subtle::NoBarrier_Load(cursor_location_memory());
  return gfx::Point(static_cast<int16_t>(location >> 16),
                    static_cast<int16_t>(location & 0xFFFF));
}

void WindowTreeClient::StartPointerWatcher(bool want_moves) {
  if (has_pointer_watcher_)
    StopPointerWatcher();
  has_pointer_watcher_ = true;
  tree_->StartPointerWatcher(want_moves);
}

void WindowTreeClient::StopPointerWatcher() {
  DCHECK(has_pointer_watcher_);
  tree_->StopPointerWatcher();
  has_pointer_watcher_ = false;
}

void WindowTreeClient::AddObserver(WindowTreeClientObserver* observer) {
  observers_.AddObserver(observer);
}

void WindowTreeClient::RemoveObserver(WindowTreeClientObserver* observer) {
  observers_.RemoveObserver(observer);
}

void WindowTreeClient::AddTestObserver(WindowTreeClientTestObserver* observer) {
  test_observers_.AddObserver(observer);
}

void WindowTreeClient::RemoveTestObserver(
    WindowTreeClientTestObserver* observer) {
  test_observers_.RemoveObserver(observer);
}

void WindowTreeClient::SetCanAcceptDrops(WindowMus* window,
                                         bool can_accept_drops) {
  DCHECK(tree_);
  tree_->SetCanAcceptDrops(window->server_id(), can_accept_drops);
}

void WindowTreeClient::SetEventTargetingPolicy(
    WindowMus* window,
    ui::mojom::EventTargetingPolicy policy) {
  DCHECK(tree_);
  tree_->SetEventTargetingPolicy(window->server_id(), policy);
}

void WindowTreeClient::OnEmbed(
    ui::mojom::WindowDataPtr root_data,
    ui::mojom::WindowTreePtr tree,
    int64_t display_id,
    ui::Id focused_window_id,
    bool drawn,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  DCHECK(!tree_ptr_);
  tree_ptr_ = std::move(tree);

  is_from_embed_ = true;
  got_initial_displays_ = true;

  if (window_manager_delegate_) {
    tree_ptr_->GetWindowManagerClient(
        MakeRequest(&window_manager_internal_client_));
    window_manager_client_ = window_manager_internal_client_.get();
  }
  OnEmbedImpl(tree_ptr_.get(), std::move(root_data), display_id,
              focused_window_id, drawn, local_surface_id);
}

void WindowTreeClient::OnEmbedFromToken(
    const base::UnguessableToken& token,
    ui::mojom::WindowDataPtr root,
    int64_t display_id,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  for (EmbedRoot* embed_root : embed_roots_) {
    if (embed_root->token() == token) {
      embed_root->OnEmbed(CreateWindowTreeHost(WindowMusType::EMBED, *root,
                                               display_id, local_surface_id));
      break;
    }
  }
}

void WindowTreeClient::OnEmbeddedAppDisconnected(ui::Id window_id) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (window)
    window->NotifyEmbeddedAppDisconnected();
}

void WindowTreeClient::OnUnembed(ui::Id window_id) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  EmbedRoot* embed_root = GetEmbedRootWithRootWindow(window->GetWindow());
  if (embed_root) {
    embed_root->OnUnembed();
    if (!GetWindowByServerId(window_id))
      return;  // EmbedRoot was deleted, resulting in deleting window.
  }

  delegate_->OnUnembed(window->GetWindow());
  delete window;
}

void WindowTreeClient::OnCaptureChanged(ui::Id new_capture_window_id,
                                        ui::Id old_capture_window_id) {
  WindowMus* new_capture_window = GetWindowByServerId(new_capture_window_id);
  WindowMus* lost_capture_window = GetWindowByServerId(old_capture_window_id);
  if (!new_capture_window && !lost_capture_window)
    return;

  InFlightCaptureChange change(this, capture_synchronizer_.get(),
                               new_capture_window);
  if (ApplyServerChangeToExistingInFlightChange(change))
    return;

  capture_synchronizer_->SetCaptureFromServer(new_capture_window);
}

void WindowTreeClient::OnFrameSinkIdAllocated(
    ui::Id window_id,
    const viz::FrameSinkId& frame_sink_id) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  window->SetFrameSinkIdFromServer(frame_sink_id);
}

void WindowTreeClient::OnTopLevelCreated(
    uint32_t change_id,
    ui::mojom::WindowDataPtr data,
    int64_t display_id,
    bool drawn,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  // The server ack'd the top level window we created and supplied the state
  // of the window at the time the server created it. For properties we do not
  // have changes in flight for we can update them immediately. For properties
  // with changes in flight we set the revert value from the server.

  if (!in_flight_map_.count(change_id)) {
    // The window may have been destroyed locally before the server could finish
    // creating the window, and before the server received the notification that
    // the window has been destroyed.
    return;
  }
  std::unique_ptr<InFlightChange> change(std::move(in_flight_map_[change_id]));
  in_flight_map_.erase(change_id);

  WindowMus* window = change->window();
  WindowTreeHostMus* window_tree_host = GetWindowTreeHostMus(window);

  // Drawn state and display-id always come from the server (they can't be
  // modified locally).
  window_tree_host->set_display_id(display_id);

  // The default visibilty is false, we only need update visibility if it
  // differs from that.
  if (data->visible) {
    InFlightVisibleChange visible_change(this, window, data->visible);
    InFlightChange* current_change =
        GetOldestInFlightChangeMatching(visible_change);
    if (current_change)
      current_change->SetRevertValueFrom(visible_change);
    else
      SetWindowVisibleFromServer(window, true);
  }

  const gfx::Rect bounds(data->bounds);
  {
    InFlightBoundsChange bounds_change(this, window, bounds, local_surface_id);
    InFlightChange* current_change =
        GetOldestInFlightChangeMatching(bounds_change);
    if (current_change) {
      current_change->SetRevertValueFrom(bounds_change);
    } else {
      const gfx::Rect window_bounds =
          is_using_pixels()
              ? gfx::ConvertRectToPixel(window->GetDeviceScaleFactor(),
                                        window->GetWindow()->bounds())
              : window->GetWindow()->bounds();
      if (window_bounds != bounds)
        SetWindowBoundsFromServer(window, bounds, local_surface_id);
    }
  }

  // There is currently no API to bulk set properties, so we iterate over each
  // property individually.
  for (const auto& pair : data->properties) {
    std::unique_ptr<std::vector<uint8_t>> revert_value(
        std::make_unique<std::vector<uint8_t>>(pair.second));
    InFlightPropertyChange property_change(window, pair.first,
                                           std::move(revert_value));
    InFlightChange* current_change =
        GetOldestInFlightChangeMatching(property_change);
    if (current_change) {
      current_change->SetRevertValueFrom(property_change);
    } else {
      window->SetPropertyFromServer(pair.first, &pair.second);
    }
  }

  // Top level windows should not have a parent.
  DCHECK_EQ(0u, data->parent_id);
}

void WindowTreeClient::OnWindowBoundsChanged(
    ui::Id window_id,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  InFlightBoundsChange new_change(this, window, new_bounds, local_surface_id);
  if (ApplyServerChangeToExistingInFlightChange(new_change))
    return;

  SetWindowBoundsFromServer(window, new_bounds, local_surface_id);
}

void WindowTreeClient::OnWindowTransformChanged(
    ui::Id window_id,
    const gfx::Transform& old_transform,
    const gfx::Transform& new_transform) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  InFlightTransformChange new_change(this, window, new_transform);
  if (ApplyServerChangeToExistingInFlightChange(new_change))
    return;

  SetWindowTransformFromServer(window, new_transform);
}

void WindowTreeClient::OnClientAreaChanged(
    ui::Id window_id,
    const gfx::Insets& new_client_area,
    const std::vector<gfx::Rect>& new_additional_client_areas) {
  // TODO(sky): this function should be removed when --mash goes away.
  // https://crbug.com/842365.
  if (!is_using_pixels())
    return;

  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  float device_scale_factor = window->GetDeviceScaleFactor();
  std::vector<gfx::Rect> new_additional_client_areas_in_dip;
  for (const gfx::Rect& area : new_additional_client_areas) {
    new_additional_client_areas_in_dip.push_back(
        gfx::ConvertRectToDIP(device_scale_factor, area));
  }
  window_manager_delegate_->OnWmSetClientArea(
      window->GetWindow(),
      gfx::ConvertInsetsToDIP(device_scale_factor, new_client_area),
      new_additional_client_areas_in_dip);
}

void WindowTreeClient::OnTransientWindowAdded(ui::Id window_id,
                                              ui::Id transient_window_id) {
  WindowMus* window = GetWindowByServerId(window_id);
  WindowMus* transient_window = GetWindowByServerId(transient_window_id);
  // window or transient_window or both may be null if a local delete occurs
  // with an in flight add from the server.
  if (window && transient_window)
    window->AddTransientChildFromServer(transient_window);
}

void WindowTreeClient::OnTransientWindowRemoved(ui::Id window_id,
                                                ui::Id transient_window_id) {
  WindowMus* window = GetWindowByServerId(window_id);
  WindowMus* transient_window = GetWindowByServerId(transient_window_id);
  // window or transient_window or both may be null if a local delete occurs
  // with an in flight delete from the server.
  if (window && transient_window)
    window->RemoveTransientChildFromServer(transient_window);
}

void WindowTreeClient::OnWindowHierarchyChanged(
    ui::Id window_id,
    ui::Id old_parent_id,
    ui::Id new_parent_id,
    std::vector<ui::mojom::WindowDataPtr> windows) {
  const bool was_window_known = GetWindowByServerId(window_id) != nullptr;

  BuildWindowTree(windows);

  // If the window was not known, then BuildWindowTree() will have created it
  // and parented the window.
  if (!was_window_known)
    return;

  WindowMus* new_parent = GetWindowByServerId(new_parent_id);
  WindowMus* old_parent = GetWindowByServerId(old_parent_id);
  WindowMus* window = GetWindowByServerId(window_id);
  if (new_parent)
    new_parent->AddChildFromServer(window);
  else
    old_parent->RemoveChildFromServer(window);
}

void WindowTreeClient::OnWindowReordered(ui::Id window_id,
                                         ui::Id relative_window_id,
                                         ui::mojom::OrderDirection direction) {
  WindowMus* window = GetWindowByServerId(window_id);
  WindowMus* relative_window = GetWindowByServerId(relative_window_id);
  WindowMus* parent = WindowMus::Get(window->GetWindow()->parent());
  if (window && relative_window && parent &&
      parent == WindowMus::Get(relative_window->GetWindow()->parent())) {
    parent->ReorderFromServer(window, relative_window, direction);
  }
}

void WindowTreeClient::OnWindowDeleted(ui::Id window_id) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  if (roots_.count(window)) {
    // Roots are associated with WindowTreeHosts. The WindowTreeHost owns the
    // root, so we have to delete the WindowTreeHost to indirectly delete the
    // Window. Clients may want to do extra processing before the delete,
    // notify the appropriate delegate to handle the deletion. Let the window
    // know it is going to be deleted so we don't callback to the server.
    window->PrepareForDestroy();
    EmbedRoot* embed_root = GetEmbedRootWithRootWindow(window->GetWindow());
    if (embed_root)
      embed_root->OnUnembed();
    else
      delegate_->OnEmbedRootDestroyed(GetWindowTreeHostMus(window));
  } else {
    window->DestroyFromServer();
  }
}

void WindowTreeClient::OnWindowVisibilityChanged(ui::Id window_id,
                                                 bool visible) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  InFlightVisibleChange new_change(this, window, visible);
  if (ApplyServerChangeToExistingInFlightChange(new_change))
    return;

  SetWindowVisibleFromServer(window, visible);
}

void WindowTreeClient::OnWindowOpacityChanged(ui::Id window_id,
                                              float old_opacity,
                                              float new_opacity) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  InFlightOpacityChange new_change(window, new_opacity);
  if (ApplyServerChangeToExistingInFlightChange(new_change))
    return;

  window->SetOpacityFromServer(new_opacity);
}

void WindowTreeClient::OnWindowParentDrawnStateChanged(ui::Id window_id,
                                                       bool drawn) {
  // TODO: route to WindowTreeHost.
  /*
  Window* window = GetWindowByServerId(window_id);
  if (window)
    WindowPrivate(window).LocalSetParentDrawn(drawn);
  */
}

void WindowTreeClient::OnWindowSharedPropertyChanged(
    ui::Id window_id,
    const std::string& name,
    const base::Optional<std::vector<uint8_t>>& transport_data) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  std::unique_ptr<std::vector<uint8_t>> data;
  if (transport_data.has_value())
    data = std::make_unique<std::vector<uint8_t>>(transport_data.value());

  InFlightPropertyChange new_change(window, name, std::move(data));
  if (ApplyServerChangeToExistingInFlightChange(new_change))
    return;

  window->SetPropertyFromServer(
      name, transport_data.has_value() ? &transport_data.value() : nullptr);
}

void WindowTreeClient::OnWindowInputEvent(
    uint32_t event_id,
    ui::Id window_id,
    int64_t display_id,
    ui::Id display_root_window_id,
    const gfx::PointF& event_location_in_screen_pixel_layout,
    std::unique_ptr<ui::Event> event,
    bool matches_pointer_watcher) {
  DCHECK(event);
  WindowMus* window = GetWindowByServerId(window_id);  // May be null.

  if (matches_pointer_watcher && has_pointer_watcher_) {
    DCHECK(event->IsPointerEvent());
    if (is_using_pixels()) {
      std::unique_ptr<ui::Event> event_in_dip(ui::Event::Clone(*event));
      ConvertPointerEventLocationToDip(display_id, window,
                                       event_in_dip->AsLocatedEvent());
      delegate_->OnPointerEventObserved(*event_in_dip->AsPointerEvent(),
                                        display_id,
                                        window ? window->GetWindow() : nullptr);
    } else {
      delegate_->OnPointerEventObserved(*event->AsPointerEvent(), display_id,
                                        window ? window->GetWindow() : nullptr);
    }
  }

  // If the window has already been deleted, use |event| to update event states
  // kept in aura::Env.
  if (!window || !window->GetWindow()->GetHost()) {
    EnvInputStateController* env_controller =
        Env::GetInstance()->env_controller();
    std::unique_ptr<ui::Event> mapped_event = MapEvent(*event.get());
    if (mapped_event->IsMouseEvent()) {
      env_controller->UpdateStateForMouseEvent(nullptr,
                                               *mapped_event->AsMouseEvent());
    } else if (mapped_event->IsTouchEvent()) {
      env_controller->UpdateStateForTouchEvent(*mapped_event->AsTouchEvent());
    }
    tree_->OnWindowInputEventAck(event_id, ui::mojom::EventResult::UNHANDLED);
    return;
  }

  if (event->IsKeyEvent()) {
    InputMethodMus* input_method = GetWindowTreeHostMus(window)->input_method();
    if (input_method) {
      ignore_result(input_method->DispatchKeyEvent(
          event->AsKeyEvent(), CreateEventResultCallback(event_id)));
      return;
    }
  }

  // TODO(moshayedi): crbug.com/617222. No need to convert to ui::MouseEvent or
  // ui::TouchEvent once we have proper support for pointer events.
  std::unique_ptr<ui::Event> mapped_event = MapEvent(*event.get());
  ui::Event* event_to_dispatch = mapped_event.get();
// Ash wants the native events in one place (see ExtendedMouseWarpController).
// By using the constructor that takes a MouseEvent we ensure the MouseEvent
// has a NativeEvent that can be used to extract the pixel coordinates.
//
// TODO: this should really be covered by |root_location|. See 608547 for
// details.
#if defined(USE_OZONE)
  // TODO(sky): remove conditional. This code was only necessary when ash used
  // this code. Now that ash does not use it, it can be removed once --mash is
  // removed. https://crbug.com/842365.
  std::unique_ptr<ui::MouseEvent> mapped_event_with_native;
  if (config_ == Config::kMash) {
    if (mapped_event->type() == ui::ET_MOUSE_MOVED ||
        mapped_event->type() == ui::ET_MOUSE_DRAGGED) {
      mapped_event_with_native = std::make_unique<ui::MouseEvent>(
          static_cast<const ui::PlatformEvent&>(mapped_event.get()));
      // MouseEvent(NativeEvent) sets the root_location to location.
      mapped_event_with_native->set_root_location_f(
          event_location_in_screen_pixel_layout);
      // |mapped_event| is now the NativeEvent. It's expected the location of
      // the NativeEvent is the same as root_location.
      mapped_event->AsMouseEvent()->set_location_f(
          event_location_in_screen_pixel_layout);
      event_to_dispatch = mapped_event_with_native.get();
    }
  }
#endif
  // |ack_handler| may use |event_to_dispatch| from its destructor, so it needs
  // to be destroyed after |event_to_dispatch| is destroyed.
  EventAckHandler ack_handler(CreateEventResultCallback(event_id));
#if defined(USE_OZONE)
  ack_handler.SetPlatformEventSourceAndEvent(platform_event_source_.get(),
                                             event_to_dispatch);
#endif

  WindowMus* display_root_window = GetWindowByServerId(display_root_window_id);
  // TODO(sky): simplify conditional. See comment in USE_OZONE above for why
  // this isn't necessary with kMus2. https://crbug.com/842365.
  if (config_ == Config::kMash && display_root_window &&
      event->IsLocatedEvent() &&
      display::Screen::GetScreen()->GetPrimaryDisplay().id() ==
          display::kUnifiedDisplayId) {
    // In Ash's unified desktop mode, each physical display mirrors part of a
    // single virtual display. Dispatch events to the root window of the mirror
    // display supplying the event, using locations relative to that display.
    // Use a null target to ensure events reach the MusUnifiedEventTargeter.
    // This paralells the behavior of unified desktop mode in classic Ash.
    ui::Event::DispatcherApi(event_to_dispatch).set_target(nullptr);
    ui::LocatedEvent* located_event = event_to_dispatch->AsLocatedEvent();
    located_event->set_location_f(located_event->root_location_f());
    window = display_root_window;
  } else if (!event->IsKeyEvent()) {
    // Set |window| as the target, except for key events. Key events go to the
    // focused window, which may have changed by the time we process the event.
    ui::Event::DispatcherApi(event_to_dispatch).set_target(window->GetWindow());
  }
#if defined(USE_OZONE)
  if (platform_event_source_)
    platform_event_source_->OnWillProcessEvent(event_to_dispatch);
#endif

  GetWindowTreeHostMus(window)->SendEventToSink(event_to_dispatch);

  ack_handler.set_handled(event_to_dispatch->handled());
}

void WindowTreeClient::OnPointerEventObserved(std::unique_ptr<ui::Event> event,
                                              ui::Id window_id,
                                              int64_t display_id) {
  DCHECK(event);
  DCHECK(event->IsPointerEvent());
  if (!has_pointer_watcher_)
    return;

  WindowMus* target_window = GetWindowByServerId(window_id);
  ConvertPointerEventLocationToDip(display_id, target_window,
                                   event->AsLocatedEvent());
  delegate_->OnPointerEventObserved(
      *event->AsPointerEvent(), display_id,
      target_window ? target_window->GetWindow() : nullptr);
}

void WindowTreeClient::OnWindowFocused(ui::Id focused_window_id) {
  WindowMus* focused_window = GetWindowByServerId(focused_window_id);
  InFlightFocusChange new_change(this, focus_synchronizer_.get(),
                                 focused_window);
  if (ApplyServerChangeToExistingInFlightChange(new_change))
    return;

  focus_synchronizer_->SetFocusFromServer(focused_window);
}

void WindowTreeClient::OnWindowCursorChanged(ui::Id window_id,
                                             ui::CursorData cursor) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  InFlightCursorChange new_change(window, cursor);
  if (ApplyServerChangeToExistingInFlightChange(new_change))
    return;

  window->SetCursorFromServer(cursor);
}

void WindowTreeClient::OnWindowSurfaceChanged(
    ui::Id window_id,
    const viz::SurfaceInfo& surface_info) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window)
    return;

  // If the parent is informed of a child's surface then that surface ID is
  // guaranteed to be available in the display compositor so we set it as the
  // fallback. If surface synchronization is enabled, the primary SurfaceInfo
  // is created by the embedder, and the LocalSurfaceId is allocated by the
  // embedder.
  window->SetFallbackSurfaceInfo(surface_info);
}

void WindowTreeClient::OnDragDropStart(
    const base::flat_map<std::string, std::vector<uint8_t>>& mime_data) {
  drag_drop_controller_->OnDragDropStart(mojo::FlatMapToMap(mime_data));
}

void WindowTreeClient::OnDragEnter(ui::Id window_id,
                                   uint32_t key_state,
                                   const gfx::Point& position,
                                   uint32_t effect_bitmask,
                                   OnDragEnterCallback callback) {
  std::move(callback).Run(drag_drop_controller_->OnDragEnter(
      GetWindowByServerId(window_id), key_state, position, effect_bitmask));
}

void WindowTreeClient::OnDragOver(ui::Id window_id,
                                  uint32_t key_state,
                                  const gfx::Point& position,
                                  uint32_t effect_bitmask,
                                  OnDragOverCallback callback) {
  std::move(callback).Run(drag_drop_controller_->OnDragOver(
      GetWindowByServerId(window_id), key_state, position, effect_bitmask));
}

void WindowTreeClient::OnDragLeave(ui::Id window_id) {
  drag_drop_controller_->OnDragLeave(GetWindowByServerId(window_id));
}

void WindowTreeClient::OnDragDropDone() {
  drag_drop_controller_->OnDragDropDone();
}

void WindowTreeClient::OnCompleteDrop(ui::Id window_id,
                                      uint32_t key_state,
                                      const gfx::Point& position,
                                      uint32_t effect_bitmask,
                                      OnCompleteDropCallback callback) {
  std::move(callback).Run(drag_drop_controller_->OnCompleteDrop(
      GetWindowByServerId(window_id), key_state, position, effect_bitmask));
}

void WindowTreeClient::OnPerformDragDropCompleted(uint32_t change_id,
                                                  bool success,
                                                  uint32_t action_taken) {
  OnChangeCompleted(change_id, success);
  if (drag_drop_controller_->DoesChangeIdMatchDragChangeId(change_id))
    drag_drop_controller_->OnPerformDragDropCompleted(action_taken);
}

void WindowTreeClient::OnChangeCompleted(uint32_t change_id, bool success) {
  std::unique_ptr<InFlightChange> change(std::move(in_flight_map_[change_id]));
  in_flight_map_.erase(change_id);
  if (!change)
    return;

  for (auto& observer : test_observers_)
    observer.OnChangeCompleted(change_id, change->change_type(), success);

  if (!success)
    change->ChangeFailed();

  InFlightChange* next_change = GetOldestInFlightChangeMatching(*change);
  if (next_change) {
    if (!success)
      next_change->SetRevertValueFrom(*change);
  } else if (!success) {
    change->Revert();
  }

  if (change_id == current_move_loop_change_) {
    current_move_loop_change_ = 0;
    on_current_move_finished_.Run(success);
    on_current_move_finished_.Reset();
  }
}

void WindowTreeClient::SetBlockingContainers(
    const std::vector<BlockingContainers>& all_blocking_containers) {
  std::vector<ui::mojom::BlockingContainersPtr>
      transport_all_blocking_containers;
  for (const BlockingContainers& blocking_containers :
       all_blocking_containers) {
    ui::mojom::BlockingContainersPtr transport_blocking_containers =
        ui::mojom::BlockingContainers::New();
    // The |system_modal_container| must be specified, |min_container| may be
    // null.
    DCHECK(blocking_containers.system_modal_container);
    transport_blocking_containers->system_modal_container_id =
        GetServerIdForWindow(blocking_containers.system_modal_container);
    transport_blocking_containers->min_container_id =
        GetServerIdForWindow(blocking_containers.min_container);
    transport_all_blocking_containers.push_back(
        std::move(transport_blocking_containers));
  }
  window_manager_client_->SetBlockingContainers(
      std::move(transport_all_blocking_containers),
      base::BindOnce(&OnAckMustSucceed, FROM_HERE));
}

void WindowTreeClient::GetWindowManager(
    mojo::AssociatedInterfaceRequest<WindowManager> internal) {
  window_manager_internal_.reset(
      new mojo::AssociatedBinding<ui::mojom::WindowManager>(
          this, std::move(internal)));
}

void WindowTreeClient::RequestClose(ui::Id window_id) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (!window || !IsRoot(window))
    return;

  // Since the window is the root window, we send close request to the entire
  // WindowTreeHost.
  GetWindowTreeHostMus(window->GetWindow())->OnCloseRequest();
}

bool WindowTreeClient::WaitForInitialDisplays() {
  if (got_initial_displays_)
    return true;

  bool valid_wait = true;
  // TODO(sky): having to block here is not ideal. http://crbug.com/594852.
  while (!got_initial_displays_ && valid_wait)
    valid_wait = binding_.WaitForIncomingMethodCall();
  return valid_wait;
}

WindowTreeHostMusInitParams WindowTreeClient::CreateInitParamsForNewDisplay() {
  WindowTreeHostMusInitParams init_params;
  init_params.window_port = std::make_unique<WindowPortMus>(
      this, WindowMusType::DISPLAY_MANUALLY_CREATED);
  roots_.insert(init_params.window_port.get());
  init_params.window_tree_client = this;
  return init_params;
}

void WindowTreeClient::OnConnect() {
  got_initial_displays_ = true;
  if (window_manager_delegate_)
    window_manager_delegate_->OnWmConnected();
}

void WindowTreeClient::WmOnAcceleratedWidgetForDisplay(
    int64_t display,
    gpu::SurfaceHandle surface_handle) {
  if (window_manager_delegate_) {
    window_manager_delegate_->OnWmAcceleratedWidgetAvailableForDisplay(
        display, surface_handle);
  }
}

void WindowTreeClient::WmNewDisplayAdded(
    const display::Display& display,
    ui::mojom::WindowDataPtr root_data,
    bool parent_drawn,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  WmNewDisplayAddedImpl(display, std::move(root_data), parent_drawn,
                        local_surface_id);
}

void WindowTreeClient::WmDisplayRemoved(int64_t display_id) {
  DCHECK(window_manager_delegate_);
  for (WindowMus* root : roots_) {
    DCHECK(root->GetWindow()->GetHost());
    WindowTreeHostMus* window_tree_host =
        static_cast<WindowTreeHostMus*>(root->GetWindow()->GetHost());
    if (window_tree_host->display_id() == display_id) {
      window_manager_delegate_->OnWmDisplayRemoved(window_tree_host);
      return;
    }
  }
}

void WindowTreeClient::WmDisplayModified(const display::Display& display) {
  DCHECK(window_manager_delegate_);
  // TODO(sky): this should likely route to WindowTreeHost.
  window_manager_delegate_->OnWmDisplayModified(display);
}

void WindowTreeClient::WmSetBounds(uint32_t change_id,
                                   ui::Id window_id,
                                   const gfx::Rect& transit_bounds_in_pixels) {
  // TODO(sky): this function should be removed when --mash goes away.
  // https://crbug.com/842365.
  WindowMus* window = GetWindowByServerId(window_id);
  if (window) {
    float device_scale_factor = window->GetDeviceScaleFactor();
    DCHECK(window_manager_delegate_);
    gfx::Rect transit_bounds_in_dip =
        gfx::ConvertRectToDIP(device_scale_factor, transit_bounds_in_pixels);
    window_manager_delegate_->OnWmSetBounds(window->GetWindow(),
                                            transit_bounds_in_dip);
  } else {
    DVLOG(1) << "Unknown window passed to WmSetBounds().";
  }
  if (window_manager_client_)
    window_manager_client_->WmSetBoundsResponse(change_id);
}

void WindowTreeClient::WmSetProperty(
    uint32_t change_id,
    ui::Id window_id,
    const std::string& name,
    const base::Optional<std::vector<uint8_t>>& transit_data) {
  WindowMus* window = GetWindowByServerId(window_id);
  bool result = false;
  if (window) {
    DCHECK(window_manager_delegate_);
    std::unique_ptr<std::vector<uint8_t>> data;
    if (transit_data.has_value())
      data.reset(new std::vector<uint8_t>(transit_data.value()));

    result = window_manager_delegate_->OnWmSetProperty(window->GetWindow(),
                                                       name, &data);
    if (result) {
      delegate_->GetPropertyConverter()->SetPropertyFromTransportValue(
          window->GetWindow(), name, data.get());
    }
  }
  if (window_manager_client_)
    window_manager_client_->WmResponse(change_id, result);
}

void WindowTreeClient::WmSetModalType(ui::Id window_id, ui::ModalType type) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (window)
    window_manager_delegate_->OnWmSetModalType(window->GetWindow(), type);
}

void WindowTreeClient::WmSetCanFocus(ui::Id window_id, bool can_focus) {
  WindowMus* window = GetWindowByServerId(window_id);
  if (window)
    window_manager_delegate_->OnWmSetCanFocus(window->GetWindow(), can_focus);
}

void WindowTreeClient::WmCreateTopLevelWindow(
    uint32_t change_id,
    const viz::FrameSinkId& frame_sink_id,
    const base::flat_map<std::string, std::vector<uint8_t>>&
        transport_properties) {
  DCHECK(frame_sink_id.is_valid());
  std::map<std::string, std::vector<uint8_t>> properties =
      mojo::FlatMapToMap(transport_properties);
  ui::mojom::WindowType window_type = GetWindowTypeFromProperties(properties);
  Window* window = window_manager_delegate_->OnWmCreateTopLevelWindow(
      window_type, &properties);
  if (!window) {
    window_manager_client_->OnWmCreatedTopLevelWindow(change_id,
                                                      kInvalidServerId);
    return;
  }
  embedded_windows_[base::checked_cast<ui::ClientSpecificId>(
                        frame_sink_id.client_id())]
      .insert(window);
  if (window_manager_client_) {
    window_manager_client_->OnWmCreatedTopLevelWindow(
        change_id, WindowMus::Get(window)->server_id());
    OnFrameSinkIdAllocated(WindowMus::Get(window)->server_id(), frame_sink_id);
  }
}

void WindowTreeClient::WmClientJankinessChanged(ui::ClientSpecificId client_id,
                                                bool janky) {
  if (window_manager_delegate_) {
    auto it = embedded_windows_.find(client_id);
    // TODO(sky): early return necessitated because of http://crbug.com/766890.
    if (it == embedded_windows_.end())
      return;
    window_manager_delegate_->OnWmClientJankinessChanged(
        embedded_windows_[client_id], janky);
  }
}

void WindowTreeClient::WmBuildDragImage(const gfx::Point& screen_location,
                                        const SkBitmap& drag_image,
                                        const gfx::Vector2d& drag_image_offset,
                                        ui::mojom::PointerKind source) {
  if (!window_manager_delegate_)
    return;

  window_manager_delegate_->OnWmBuildDragImage(screen_location, drag_image,
                                               drag_image_offset, source);
}

void WindowTreeClient::WmMoveDragImage(const gfx::Point& screen_location,
                                       WmMoveDragImageCallback callback) {
  if (window_manager_delegate_)
    window_manager_delegate_->OnWmMoveDragImage(screen_location);
  std::move(callback).Run();
}

void WindowTreeClient::WmDestroyDragImage() {
  if (!window_manager_delegate_)
    return;

  window_manager_delegate_->OnWmDestroyDragImage();
}

void WindowTreeClient::WmPerformMoveLoop(uint32_t change_id,
                                         ui::Id window_id,
                                         ui::mojom::MoveLoopSource source,
                                         const gfx::Point& cursor_location) {
  if (!window_manager_delegate_ || current_wm_move_loop_change_ != 0) {
    OnWmMoveLoopCompleted(change_id, false);
    return;
  }

  current_wm_move_loop_change_ = change_id;
  current_wm_move_loop_window_id_ = window_id;
  WindowMus* window = GetWindowByServerId(window_id);
  if (window) {
    window_manager_delegate_->OnWmPerformMoveLoop(
        window->GetWindow(), source, cursor_location,
        base::Bind(&WindowTreeClient::OnWmMoveLoopCompleted,
                   weak_factory_.GetWeakPtr(), change_id));
  } else {
    OnWmMoveLoopCompleted(change_id, false);
  }
}

void WindowTreeClient::WmCancelMoveLoop(uint32_t change_id) {
  if (!window_manager_delegate_ || change_id != current_wm_move_loop_change_)
    return;

  WindowMus* window = GetWindowByServerId(current_wm_move_loop_window_id_);
  if (window)
    window_manager_delegate_->OnWmCancelMoveLoop(window->GetWindow());
}

void WindowTreeClient::WmDeactivateWindow(ui::Id window_id) {
  if (!window_manager_delegate_)
    return;

  WindowMus* window = GetWindowByServerId(window_id);
  if (!window) {
    DVLOG(1) << "Attempt to deactivate invalid window " << window_id;
    return;
  }

  if (!window_manager_delegate_->IsWindowActive(window->GetWindow())) {
    DVLOG(1) << "Non-active window requested deactivation.";
    return;
  }

  window_manager_delegate_->OnWmDeactivateWindow(window->GetWindow());
}

void WindowTreeClient::WmStackAbove(uint32_t wm_change_id,
                                    ui::Id above_id,
                                    ui::Id below_id) {
  if (!window_manager_delegate_)
    return;

  WindowMus* below_mus = GetWindowByServerId(below_id);
  if (!below_mus) {
    DVLOG(1) << "Attempt to stack at top invalid window " << below_id;
    if (window_manager_client_)
      window_manager_client_->WmResponse(wm_change_id, false);
    return;
  }

  WindowMus* above_mus = GetWindowByServerId(above_id);
  if (!above_mus) {
    DVLOG(1) << "Attempt to stack at top invalid window " << above_id;
    if (window_manager_client_)
      window_manager_client_->WmResponse(wm_change_id, false);
    return;
  }

  Window* above = above_mus->GetWindow();
  Window* below = below_mus->GetWindow();

  if (above->parent() != below->parent()) {
    DVLOG(1) << "Windows do not share the same parent";
    if (window_manager_client_)
      window_manager_client_->WmResponse(wm_change_id, false);
    return;
  }

  above->parent()->StackChildAbove(above, below);

  if (window_manager_client_)
    window_manager_client_->WmResponse(wm_change_id, true);
}

void WindowTreeClient::WmStackAtTop(uint32_t wm_change_id, ui::Id window_id) {
  if (!window_manager_delegate_)
    return;

  WindowMus* window = GetWindowByServerId(window_id);
  if (!window) {
    DVLOG(1) << "Attempt to stack at top invalid window " << window_id;
    if (window_manager_client_)
      window_manager_client_->WmResponse(wm_change_id, false);
    return;
  }

  Window* parent = window->GetWindow()->parent();
  parent->StackChildAtTop(window->GetWindow());

  if (window_manager_client_)
    window_manager_client_->WmResponse(wm_change_id, true);
}

void WindowTreeClient::WmPerformWmAction(ui::Id window_id,
                                         const std::string& action) {
  if (!window_manager_delegate_)
    return;

  WindowMus* window = GetWindowByServerId(window_id);
  if (window)
    window_manager_delegate_->OnWmPerformAction(window->GetWindow(), action);
}

void WindowTreeClient::OnAccelerator(uint32_t ack_id,
                                     uint32_t accelerator_id,
                                     std::unique_ptr<ui::Event> event) {
  DCHECK(event);
  base::flat_map<std::string, std::vector<uint8_t>> properties;
  const ui::mojom::EventResult result = window_manager_delegate_->OnAccelerator(
      accelerator_id, *event.get(), &properties);
  if (ack_id && window_manager_client_)
    window_manager_client_->OnAcceleratorAck(ack_id, result, properties);
}

void WindowTreeClient::OnCursorTouchVisibleChanged(bool enabled) {
  if (window_manager_client_)
    window_manager_delegate_->OnCursorTouchVisibleChanged(enabled);
}

void WindowTreeClient::OnEventBlockedByModalWindow(ui::Id window_id) {
  if (!window_manager_delegate_)
    return;

  WindowMus* window = GetWindowByServerId(window_id);
  if (window)
    window_manager_delegate_->OnEventBlockedByModalWindow(window->GetWindow());
}

void WindowTreeClient::SetFrameDecorationValues(
    ui::mojom::FrameDecorationValuesPtr values) {
  if (window_manager_client_) {
    normal_client_area_insets_ = values->normal_client_area_insets;
    window_manager_client_->WmSetFrameDecorationValues(std::move(values));
  }
}

void WindowTreeClient::SetNonClientCursor(Window* window,
                                          const ui::CursorData& cursor) {
  if (window_manager_client_) {
    window_manager_client_->WmSetNonClientCursor(
        WindowMus::Get(window)->server_id(), cursor);
  }
}

void WindowTreeClient::AddAccelerators(
    std::vector<ui::mojom::WmAcceleratorPtr> accelerators,
    const base::Callback<void(bool)>& callback) {
  if (window_manager_client_) {
    window_manager_client_->AddAccelerators(std::move(accelerators), callback);
  }
}

void WindowTreeClient::RemoveAccelerator(uint32_t id) {
  if (window_manager_client_) {
    window_manager_client_->RemoveAccelerator(id);
  }
}

void WindowTreeClient::AddActivationParent(Window* window) {
  if (window_manager_client_) {
    window_manager_client_->AddActivationParent(
        WindowMus::Get(window)->server_id());
  }
}

void WindowTreeClient::RemoveActivationParent(Window* window) {
  if (window_manager_client_) {
    window_manager_client_->RemoveActivationParent(
        WindowMus::Get(window)->server_id());
  }
}

void WindowTreeClient::SetExtendedHitRegionForChildren(
    Window* window,
    const gfx::Insets& mouse_insets,
    const gfx::Insets& touch_insets) {
  if (!window_manager_client_)
    return;

  // TODO(sky): this function should be removed when --mash goes away.
  // https://crbug.com/842365.
  WindowMus* window_mus = WindowMus::Get(window);
  const float device_scale_factor = window_mus->GetDeviceScaleFactor();
  window_manager_client_->SetExtendedHitRegionForChildren(
      window_mus->server_id(),
      gfx::ConvertInsetsToPixel(device_scale_factor, mouse_insets),
      gfx::ConvertInsetsToPixel(device_scale_factor, touch_insets));
}

void WindowTreeClient::LockCursor() {
  if (window_manager_client_)
    window_manager_client_->WmLockCursor();
}

void WindowTreeClient::UnlockCursor() {
  if (window_manager_client_)
    window_manager_client_->WmUnlockCursor();
}

void WindowTreeClient::SetCursorVisible(bool visible) {
  if (window_manager_client_)
    window_manager_client_->WmSetCursorVisible(visible);
}

void WindowTreeClient::SetCursorSize(ui::CursorSize cursor_size) {
  if (window_manager_client_)
    window_manager_client_->WmSetCursorSize(cursor_size);
}

void WindowTreeClient::SetGlobalOverrideCursor(
    base::Optional<ui::CursorData> cursor) {
  if (window_manager_client_)
    window_manager_client_->WmSetGlobalOverrideCursor(std::move(cursor));
}

void WindowTreeClient::SetCursorTouchVisible(bool enabled) {
  if (window_manager_client_)
    window_manager_client_->WmSetCursorTouchVisible(enabled);
}

void WindowTreeClient::InjectEvent(const ui::Event& event, int64_t display_id) {
  if (!event_injector_)
    connector()->BindInterface(ui::mojom::kServiceName, &event_injector_);
  // Check event_injector_ so we don't crash if access to the interface was
  // refused.
  if (event_injector_) {
    event_injector_->InjectEvent(display_id, ui::Event::Clone(event),
                                 base::DoNothing());
  }
}

void WindowTreeClient::SetKeyEventsThatDontHideCursor(
    std::vector<ui::mojom::EventMatcherPtr> cursor_key_list) {
  if (window_manager_client_) {
    window_manager_client_->SetKeyEventsThatDontHideCursor(
        std::move(cursor_key_list));
  }
}

void WindowTreeClient::RequestClose(Window* window) {
  DCHECK(window);
  if (window_manager_client_)
    window_manager_client_->WmRequestClose(WindowMus::Get(window)->server_id());
}

void WindowTreeClient::SetDisplayConfiguration(
    const std::vector<display::Display>& displays,
    std::vector<ui::mojom::WmViewportMetricsPtr> viewport_metrics,
    int64_t primary_display_id,
    const std::vector<display::Display>& mirrors) {
  // TODO(sky): this function should be removed when --mash goes away.
  // https://crbug.com/842365.
  DCHECK_EQ(displays.size() + mirrors.size(), viewport_metrics.size());
  if (window_manager_client_) {
    const int64_t internal_display_id =
        display::Display::HasInternalDisplay()
            ? display::Display::InternalDisplayId()
            : display::kInvalidDisplayId;
    window_manager_client_->SetDisplayConfiguration(
        displays, std::move(viewport_metrics), primary_display_id,
        internal_display_id, mirrors,
        base::BindOnce(&OnAckMustSucceed, FROM_HERE));
  }
}

void WindowTreeClient::AddDisplayReusingWindowTreeHost(
    WindowTreeHostMus* window_tree_host,
    const display::Display& display,
    ui::mojom::WmViewportMetricsPtr viewport_metrics) {
  // TODO(sky): this function should be removed when --mash goes away.
  // https://crbug.com/842365.
  DCHECK_NE(display.id(), window_tree_host->display_id());
  window_tree_host->set_display_id(display.id());
  if (window_manager_client_) {
    // NOTE: The values of |is_primary_display| and |mirrors| do not matter,
    // because SetDisplayConfiguration() is called shortly after this completes.
    const bool is_primary_display = true;
    std::vector<display::Display> mirrors;
    WindowMus* display_root_window = WindowMus::Get(window_tree_host->window());
    window_manager_client_->SetDisplayRoot(
        display, std::move(viewport_metrics), is_primary_display,
        display_root_window->server_id(), mirrors,
        base::BindOnce(&OnAckMustSucceed, FROM_HERE));
    window_tree_host->compositor()->SetLocalSurfaceId(
        display_root_window->GetOrAllocateLocalSurfaceId(
            window_tree_host->GetBoundsInPixels().size()));
  }
}

void WindowTreeClient::SwapDisplayRoots(WindowTreeHostMus* window_tree_host1,
                                        WindowTreeHostMus* window_tree_host2) {
  DCHECK_NE(window_tree_host1, window_tree_host2);
  const int64_t display_id1 = window_tree_host1->display_id();
  const int64_t display_id2 = window_tree_host2->display_id();
  DCHECK_NE(display_id1, display_id2);
  window_tree_host1->set_display_id(display_id2);
  window_tree_host2->set_display_id(display_id1);

  // Swap the accelerated widgets so each host paints to the correct display.
  gfx::AcceleratedWidget widget1 = window_tree_host1->GetAcceleratedWidget();
  gfx::AcceleratedWidget widget2 = window_tree_host2->GetAcceleratedWidget();
  window_tree_host1->OverrideAcceleratedWidget(widget2);
  window_tree_host2->OverrideAcceleratedWidget(widget1);

  if (window_manager_client_) {
    window_manager_client_->SwapDisplayRoots(
        display_id1, display_id2, base::BindOnce(&OnAckMustSucceed, FROM_HERE));
  }
}

void WindowTreeClient::OnWindowTreeHostBoundsWillChange(
    WindowTreeHostMus* window_tree_host,
    const gfx::Rect& bounds_in_pixels) {
  gfx::Rect old_bounds = window_tree_host->GetBoundsInPixels();
  gfx::Rect new_bounds = bounds_in_pixels;
  if (!is_using_pixels()) {
    const float device_scale_factor = window_tree_host->device_scale_factor();
    old_bounds = gfx::ConvertRectToDIP(device_scale_factor, old_bounds);
    new_bounds = gfx::ConvertRectToDIP(device_scale_factor, new_bounds);
  }
  if (window_manager_delegate_) {
    // The window manager origins should always be 0x0. The real origin is
    // communicated by way of SetDisplayConfiguration().
    old_bounds.set_origin(gfx::Point());
    new_bounds.set_origin(gfx::Point());
  }
  ScheduleInFlightBoundsChange(WindowMus::Get(window_tree_host->window()),
                               old_bounds, new_bounds);
}

void WindowTreeClient::OnWindowTreeHostClientAreaWillChange(
    WindowTreeHostMus* window_tree_host,
    const gfx::Insets& client_area,
    const std::vector<gfx::Rect>& additional_client_areas) {
  DCHECK(tree_);
  WindowMus* window = WindowMus::Get(window_tree_host->window());
  if (is_using_pixels()) {
    const float device_scale_factor = window->GetDeviceScaleFactor();
    std::vector<gfx::Rect> additional_client_areas_in_pixels;
    for (const gfx::Rect& area : additional_client_areas) {
      additional_client_areas_in_pixels.push_back(
          gfx::ConvertRectToPixel(device_scale_factor, area));
    }
    tree_->SetClientArea(
        window->server_id(),
        gfx::ConvertInsetsToPixel(device_scale_factor, client_area),
        additional_client_areas_in_pixels);
  } else {
    tree_->SetClientArea(window->server_id(), client_area,
                         additional_client_areas);
  }
}

void WindowTreeClient::OnWindowTreeHostSetOpacity(
    WindowTreeHostMus* window_tree_host,
    float opacity) {
  WindowMus* window = WindowMus::Get(window_tree_host->window());
  const uint32_t change_id = ScheduleInFlightChange(
      std::make_unique<CrashInFlightChange>(window, ChangeType::OPACITY));
  tree_->SetWindowOpacity(change_id, window->server_id(), opacity);
}

void WindowTreeClient::OnWindowTreeHostDeactivateWindow(
    WindowTreeHostMus* window_tree_host) {
  tree_->DeactivateWindow(
      WindowMus::Get(window_tree_host->window())->server_id());
}

void WindowTreeClient::OnWindowTreeHostStackAbove(
    WindowTreeHostMus* window_tree_host,
    Window* window) {
  WindowMus* above = WindowMus::Get(window_tree_host->window());
  WindowMus* below = WindowMus::Get(window);
  const uint32_t change_id = ScheduleInFlightChange(
      std::make_unique<CrashInFlightChange>(above, ChangeType::REORDER));
  tree_->StackAbove(change_id, above->server_id(), below->server_id());
}

void WindowTreeClient::OnWindowTreeHostStackAtTop(
    WindowTreeHostMus* window_tree_host) {
  WindowMus* window = WindowMus::Get(window_tree_host->window());
  const uint32_t change_id = ScheduleInFlightChange(
      std::make_unique<CrashInFlightChange>(window, ChangeType::REORDER));
  tree_->StackAtTop(change_id, window->server_id());
}

void WindowTreeClient::OnWindowTreeHostPerformWmAction(
    WindowTreeHostMus* window_tree_host,
    const std::string& action) {
  WindowMus* window = WindowMus::Get(window_tree_host->window());
  tree_->PerformWmAction(window->server_id(), action);
}

void WindowTreeClient::OnWindowTreeHostPerformWindowMove(
    WindowTreeHostMus* window_tree_host,
    ui::mojom::MoveLoopSource source,
    const gfx::Point& cursor_location,
    const base::Callback<void(bool)>& callback) {
  DCHECK(on_current_move_finished_.is_null());
  on_current_move_finished_ = callback;

  WindowMus* window_mus = WindowMus::Get(window_tree_host->window());
  current_move_loop_change_ = ScheduleInFlightChange(
      std::make_unique<InFlightDragChange>(window_mus, ChangeType::MOVE_LOOP));
  // Tell the window manager to take over moving us.
  tree_->PerformWindowMove(current_move_loop_change_, window_mus->server_id(),
                           source, cursor_location);
}

void WindowTreeClient::OnWindowTreeHostCancelWindowMove(
    WindowTreeHostMus* window_tree_host) {
  tree_->CancelWindowMove(
      WindowMus::Get(window_tree_host->window())->server_id());
}

void WindowTreeClient::OnWindowTreeHostMoveCursorToDisplayLocation(
    const gfx::Point& location_in_pixels,
    int64_t display_id) {
  DCHECK(window_manager_client_);
  if (window_manager_client_) {
    window_manager_client_->WmMoveCursorToDisplayLocation(location_in_pixels,
                                                          display_id);
  }
}

void WindowTreeClient::OnWindowTreeHostConfineCursorToBounds(
    const gfx::Rect& bounds_in_pixels,
    int64_t display_id) {
  DCHECK(window_manager_client_);
  if (window_manager_client_) {
    window_manager_client_->WmConfineCursorToBounds(bounds_in_pixels,
                                                    display_id);
  }
}

std::unique_ptr<WindowPortMus> WindowTreeClient::CreateWindowPortForTopLevel(
    const std::map<std::string, std::vector<uint8_t>>* properties) {
  std::unique_ptr<WindowPortMus> window_port =
      std::make_unique<WindowPortMus>(this, WindowMusType::TOP_LEVEL);
  roots_.insert(window_port.get());

  window_port->set_server_id(next_window_id_++);
  RegisterWindowMus(window_port.get());

  base::flat_map<std::string, std::vector<uint8_t>> transport_properties;
  if (properties) {
    for (const auto& property_pair : *properties)
      transport_properties[property_pair.first] = property_pair.second;
  }

  const uint32_t change_id =
      ScheduleInFlightChange(std::make_unique<CrashInFlightChange>(
          window_port.get(), ChangeType::NEW_TOP_LEVEL_WINDOW));
  tree_->NewTopLevelWindow(change_id, window_port->server_id(),
                           transport_properties);
  return window_port;
}

void WindowTreeClient::OnWindowTreeHostCreated(
    WindowTreeHostMus* window_tree_host) {
  // All WindowTreeHosts are destroyed before this, so we don't need to unset
  // the DragDropClient.
  client::SetDragDropClient(window_tree_host->window(),
                            drag_drop_controller_.get());
}

void WindowTreeClient::OnTransientChildWindowAdded(Window* parent,
                                                   Window* transient_child) {
  // TransientWindowClient is a singleton and we allow multiple
  // WindowTreeClients. Ignore changes to windows we don't know about (assume
  // they came from another connection).
  if (!IsWindowKnown(parent) || !IsWindowKnown(transient_child))
    return;

  if (WindowMus::Get(parent)->OnTransientChildAdded(
          WindowMus::Get(transient_child)) == WindowMus::ChangeSource::SERVER) {
    return;
  }

  // The change originated from client code and needs to be sent to the server.
  DCHECK(tree_);
  WindowMus* parent_mus = WindowMus::Get(parent);
  const uint32_t change_id =
      ScheduleInFlightChange(std::make_unique<CrashInFlightChange>(
          parent_mus, ChangeType::ADD_TRANSIENT_WINDOW));
  tree_->AddTransientWindow(change_id, parent_mus->server_id(),
                            WindowMus::Get(transient_child)->server_id());
}

void WindowTreeClient::OnTransientChildWindowRemoved(Window* parent,
                                                     Window* transient_child) {
  // See comments in OnTransientChildWindowAdded() for details on early return.
  if (!IsWindowKnown(parent) || !IsWindowKnown(transient_child))
    return;

  if (WindowMus::Get(parent)->OnTransientChildRemoved(
          WindowMus::Get(transient_child)) == WindowMus::ChangeSource::SERVER) {
    return;
  }
  // The change originated from client code and needs to be sent to the server.
  DCHECK(tree_);
  WindowMus* child_mus = WindowMus::Get(transient_child);
  const uint32_t change_id =
      ScheduleInFlightChange(std::make_unique<CrashInFlightChange>(
          child_mus, ChangeType::REMOVE_TRANSIENT_WINDOW_FROM_PARENT));
  tree_->RemoveTransientWindowFromParent(change_id, child_mus->server_id());
}

uint32_t WindowTreeClient::CreateChangeIdForDrag(WindowMus* window) {
  return ScheduleInFlightChange(
      std::make_unique<InFlightDragChange>(window, ChangeType::DRAG_LOOP));
}

uint32_t WindowTreeClient::CreateChangeIdForCapture(WindowMus* window) {
  return ScheduleInFlightChange(std::make_unique<InFlightCaptureChange>(
      this, capture_synchronizer_.get(), window));
}

uint32_t WindowTreeClient::CreateChangeIdForFocus(WindowMus* window) {
  return ScheduleInFlightChange(std::make_unique<InFlightFocusChange>(
      this, focus_synchronizer_.get(), window));
}

}  // namespace aura
