// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/env.h"

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/env_input_state_controller.h"
#include "ui/aura/env_observer.h"
#include "ui/aura/input_state_lookup.h"
#include "ui/aura/local/window_port_local.h"
#include "ui/aura/mouse_location_manager.h"
#include "ui/aura/mus/mus_types.h"
#include "ui/aura/mus/os_exchange_data_provider_mus.h"
#include "ui/aura/mus/system_input_injector_mus.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher_observer.h"
#include "ui/aura/window_port_for_shutdown.h"
#include "ui/events/event_target_iterator.h"
#include "ui/events/platform/platform_event_source.h"

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/ozone_switches.h"
#endif

namespace aura {

namespace {

// Env is thread local so that aura may be used on multiple threads.
base::LazyInstance<base::ThreadLocalPointer<Env>>::Leaky lazy_tls_ptr =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// Env, public:

Env::~Env() {
  if (is_os_exchange_data_provider_factory_)
    ui::OSExchangeDataProviderFactory::SetFactory(nullptr);
  if (is_override_input_injector_factory_)
    ui::SetSystemInputInjectorFactory(nullptr);

  for (EnvObserver& observer : observers_)
    observer.OnWillDestroyEnv();

#if defined(USE_OZONE)
  if (mode_ == Mode::LOCAL)
    ui::OzonePlatform::Shutdown();
#endif

  DCHECK_EQ(this, lazy_tls_ptr.Pointer()->Get());
  lazy_tls_ptr.Pointer()->Set(NULL);
}

// static
std::unique_ptr<Env> Env::CreateInstance(Mode mode) {
  DCHECK(!lazy_tls_ptr.Pointer()->Get());
  std::unique_ptr<Env> env(new Env(mode));
  env->Init();
  return env;
}

// static
Env* Env::GetInstance() {
  Env* env = lazy_tls_ptr.Pointer()->Get();
  DCHECK(env) << "Env::CreateInstance must be called before getting the "
                 "instance of Env.";
  return env;
}

// static
Env* Env::GetInstanceDontCreate() {
  return lazy_tls_ptr.Pointer()->Get();
}

std::unique_ptr<WindowPort> Env::CreateWindowPort(Window* window) {
  if (mode_ == Mode::LOCAL)
    return std::make_unique<WindowPortLocal>(window);

  if (in_mus_shutdown_)
    return std::make_unique<WindowPortForShutdown>();

  DCHECK(window_tree_client_);
  WindowMusType window_mus_type;
  switch (window->GetProperty(aura::client::kEmbedType)) {
    case aura::client::WindowEmbedType::NONE:
      window_mus_type = WindowMusType::LOCAL;
      break;
    case aura::client::WindowEmbedType::TOP_LEVEL_IN_WM:
      window_mus_type = WindowMusType::TOP_LEVEL_IN_WM;
      break;
    case aura::client::WindowEmbedType::EMBED_IN_OWNER:
      window_mus_type = WindowMusType::EMBED_IN_OWNER;
      break;
    default:
      NOTREACHED();
  }
  // Use LOCAL as all other cases are created by WindowTreeClient explicitly.
  return std::make_unique<WindowPortMus>(window_tree_client_, window_mus_type);
}

void Env::AddObserver(EnvObserver* observer) {
  observers_.AddObserver(observer);
}

void Env::RemoveObserver(EnvObserver* observer) {
  observers_.RemoveObserver(observer);
}

void Env::AddWindowEventDispatcherObserver(
    WindowEventDispatcherObserver* observer) {
  window_event_dispatcher_observers_.AddObserver(observer);
}

void Env::RemoveWindowEventDispatcherObserver(
    WindowEventDispatcherObserver* observer) {
  window_event_dispatcher_observers_.RemoveObserver(observer);
}

bool Env::IsMouseButtonDown() const {
  return input_state_lookup_.get() ? input_state_lookup_->IsMouseButtonDown() :
      mouse_button_flags_ != 0;
}

const gfx::Point& Env::last_mouse_location() const {
  if (mode_ == Mode::LOCAL || always_use_last_mouse_location_ ||
      !get_last_mouse_location_from_mus_) {
    return last_mouse_location_;
  }

  // Some tests may not install a WindowTreeClient, and we allow multiple
  // WindowTreeClients for the case of multiple connections, and this may be
  // called during shutdown, when there is no WindowTreeClient.
  if (window_tree_client_)
    last_mouse_location_ = window_tree_client_->GetCursorScreenPoint();
  return last_mouse_location_;
}

void Env::SetLastMouseLocation(const gfx::Point& last_mouse_location) {
  last_mouse_location_ = last_mouse_location;
  if (mouse_location_manager_)
    mouse_location_manager_->SetMouseLocation(last_mouse_location);
}

void Env::CreateMouseLocationManager() {
  if (!mouse_location_manager_)
    mouse_location_manager_ = std::make_unique<MouseLocationManager>();
}

mojo::ScopedSharedBufferHandle Env::GetLastMouseLocationMemory() {
  DCHECK(mouse_location_manager_);
  return mouse_location_manager_->GetMouseLocationMemory();
}

void Env::SetWindowTreeClient(WindowTreeClient* window_tree_client) {
  // The WindowTreeClient should only be set once. Test code may need to change
  // the value after the fact, to do that use EnvTestHelper.
  DCHECK(!window_tree_client_);
  window_tree_client_ = window_tree_client;
}

void Env::ScheduleEmbed(
    ui::mojom::WindowTreeClientPtr client,
    base::OnceCallback<void(const base::UnguessableToken&)> callback) {
  DCHECK_EQ(Mode::MUS, mode_);
  DCHECK(window_tree_client_);
  window_tree_client_->ScheduleEmbed(std::move(client), std::move(callback));
}

////////////////////////////////////////////////////////////////////////////////
// Env, private:

// static
bool Env::initial_throttle_input_on_resize_ = true;

Env::Env(Mode mode)
    : mode_(mode),
      env_controller_(new EnvInputStateController),
      mouse_button_flags_(0),
      is_touch_down_(false),
      get_last_mouse_location_from_mus_(mode_ == Mode::MUS),
      input_state_lookup_(InputStateLookup::Create()),
      context_factory_(nullptr),
      context_factory_private_(nullptr) {
  DCHECK(lazy_tls_ptr.Pointer()->Get() == NULL);
  lazy_tls_ptr.Pointer()->Set(this);
}

void Env::Init() {
  if (mode_ == Mode::MUS) {
    EnableMusOSExchangeDataProvider();
    EnableMusOverrideInputInjector();
    return;
  }

#if defined(USE_OZONE)
  // The ozone platform can provide its own event source. So initialize the
  // platform before creating the default event source. If running inside mus
  // let the mus process initialize ozone instead.
  ui::OzonePlatform::InitParams params;
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  // TODO(kylechar): Pass in single process information to Env::CreateInstance()
  // instead of checking flags here.
  params.single_process = command_line->HasSwitch("single-process") ||
                          command_line->HasSwitch("in-process-gpu");
  params.using_mojo = command_line->HasSwitch(switches::kEnableDrmMojo);

  ui::OzonePlatform::InitializeForUI(params);
#endif
  if (!ui::PlatformEventSource::GetInstance())
    event_source_ = ui::PlatformEventSource::CreateDefault();
}

void Env::EnableMusOSExchangeDataProvider() {
  if (!is_os_exchange_data_provider_factory_) {
    ui::OSExchangeDataProviderFactory::SetFactory(this);
    is_os_exchange_data_provider_factory_ = true;
  }
}

void Env::EnableMusOverrideInputInjector() {
  if (!is_override_input_injector_factory_) {
    ui::SetSystemInputInjectorFactory(this);
    is_override_input_injector_factory_ = true;
  }
}

void Env::NotifyWindowInitialized(Window* window) {
  for (EnvObserver& observer : observers_)
    observer.OnWindowInitialized(window);
}

void Env::NotifyHostInitialized(WindowTreeHost* host) {
  for (EnvObserver& observer : observers_)
    observer.OnHostInitialized(host);
}

void Env::NotifyHostActivated(WindowTreeHost* host) {
  for (EnvObserver& observer : observers_)
    observer.OnHostActivated(host);
}

void Env::WindowTreeClientDestroyed(aura::WindowTreeClient* client) {
  DCHECK_EQ(Mode::MUS, mode_);

  if (client != window_tree_client_)
    return;

  in_mus_shutdown_ = true;
  window_tree_client_ = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// Env, ui::EventTarget implementation:

bool Env::CanAcceptEvent(const ui::Event& event) {
  return true;
}

ui::EventTarget* Env::GetParentTarget() {
  return NULL;
}

std::unique_ptr<ui::EventTargetIterator> Env::GetChildIterator() const {
  return nullptr;
}

ui::EventTargeter* Env::GetEventTargeter() {
  NOTREACHED();
  return NULL;
}

std::unique_ptr<ui::OSExchangeData::Provider> Env::BuildProvider() {
  return std::make_unique<aura::OSExchangeDataProviderMus>();
}

std::unique_ptr<ui::SystemInputInjector> Env::CreateSystemInputInjector() {
  return std::make_unique<SystemInputInjectorMus>(window_tree_client_);
}

}  // namespace aura
