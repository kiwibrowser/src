// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mash/simple_wm/simple_wm.h"

#include <memory>

#include "base/observer_list.h"
#include "base/strings/utf_string_conversions.h"
#include "mash/simple_wm/move_event_handler.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/layout_manager.h"
#include "ui/display/screen_base.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/mojo/geometry.mojom.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/mus/aura_init.h"
#include "ui/views/mus/mus_client.h"
#include "ui/views/widget/native_widget_aura.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/focus_controller.h"

namespace simple_wm {

namespace {

const int kNonClientTopHeight = 24;
const int kNonClientSize = 5;
const int kNonClientMaximizedTopOverlap = 4;

}  // namespace

class SimpleWM::WindowListModelObserver {
 public:
  virtual void OnWindowAddedOrRemoved() = 0;
  virtual void OnWindowTitleChanged(size_t index,
                                    const base::string16& title) = 0;
};

class SimpleWM::WindowListModel : public aura::WindowObserver {
 public:
  explicit WindowListModel(aura::Window* window_container)
      : window_container_(window_container) {
    window_container_->AddObserver(this);
  }
  ~WindowListModel() override {
    window_container_->RemoveObserver(this);
    for (auto* window : windows_)
      window->RemoveObserver(this);
  }

  size_t GetSize() const {
    return windows_.size();
  }
  base::string16 GetTitle(size_t index) const {
    return windows_.at(index)->GetTitle();
  }
  aura::Window* GetWindow(size_t index) const {
    return windows_.at(index);
  }

  void AddObserver(WindowListModelObserver* observer) {
    observers_.AddObserver(observer);
  }
  void RemoveObserver(WindowListModelObserver* observer) {
    observers_.RemoveObserver(observer);
  }

 private:
  // aura::WindowObserver:
  void OnWindowAdded(aura::Window* window) override {
    if (window->parent() == window_container_)
      AddWindow(window);
    for (auto& observer : observers_)
      observer.OnWindowAddedOrRemoved();
  }
  void OnWillRemoveWindow(aura::Window* window) override {
    window->RemoveObserver(this);
    for (auto& observer : observers_)
      observer.OnWindowAddedOrRemoved();
    auto it = std::find(windows_.begin(), windows_.end(), window);
    DCHECK(it != windows_.end());
    windows_.erase(it);
  }
  void OnWindowTitleChanged(aura::Window* window) override {
    auto it = std::find(windows_.begin(), windows_.end(), window);
    size_t index = it - windows_.begin();
    for (auto& observer : observers_)
      observer.OnWindowTitleChanged(index, window->GetTitle());
  }

  void AddWindow(aura::Window* window) {
    window->AddObserver(this);
    auto it = std::find(windows_.begin(), windows_.end(), window);
    DCHECK(it == windows_.end());
    windows_.push_back(window);
  }

  aura::Window* window_container_;
  std::vector<aura::Window*> windows_;
  base::ObserverList<WindowListModelObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(WindowListModel);
};

class SimpleWM::WindowListView : public views::WidgetDelegateView,
                                 public views::ButtonListener,
                                 public SimpleWM::WindowListModelObserver {
 public:
  using ActivateCallback = base::Callback<void(aura::Window*)>;

  WindowListView(WindowListModel* model, ActivateCallback activate_callback)
      : model_(model), activate_callback_(activate_callback) {
    model_->AddObserver(this);
    Rebuild();
  }
  ~WindowListView() override {
    model_->RemoveObserver(this);
  }

  static const int kButtonSpacing = 5;

  // views::View
  void Layout() override {
    int x_offset = kButtonSpacing;
    for (int i = 0; i < child_count(); ++i) {
      View* v = child_at(i);
      gfx::Size ps = v->GetPreferredSize();
      gfx::Rect bounds(x_offset, kButtonSpacing, ps.width(), ps.height());
      v->SetBoundsRect(bounds);
      x_offset = bounds.right() + kButtonSpacing;
    }
  }
  void OnPaint(gfx::Canvas* canvas) override {
    canvas->DrawColor(SK_ColorLTGRAY);
    gfx::Rect stroke_bounds = GetLocalBounds();
    stroke_bounds.set_height(1);
    canvas->FillRect(stroke_bounds, SK_ColorDKGRAY);
  }
  gfx::Size CalculatePreferredSize() const override {
    std::unique_ptr<views::MdTextButton> measure_button(
        views::MdTextButton::Create(nullptr, base::UTF8ToUTF16("Sample")));
    int height =
        measure_button->GetPreferredSize().height() + 2 * kButtonSpacing;
    return gfx::Size(0, height);
  }

 private:
  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {
    activate_callback_.Run(
        model_->GetWindow(static_cast<size_t>(sender->tag())));
  }

  // WindowListModelObserver:
  void OnWindowAddedOrRemoved() override {
    Rebuild();
  }
  void OnWindowTitleChanged(size_t index,
                            const base::string16& new_title) override {
    views::MdTextButton* label =
        static_cast<views::MdTextButton*>(child_at(static_cast<int>(index)));
    label->SetText(new_title);
    Layout();
  }

  void Rebuild() {
    RemoveAllChildViews(true);

    size_t size = model_->GetSize();
    for (size_t i = 0; i < size; ++i) {
      base::string16 title = model_->GetTitle(i);
      if (title.empty())
        title = base::UTF8ToUTF16("Untitled");
      views::MdTextButton* button = views::MdTextButton::Create(this, title);
      button->set_tag(static_cast<int>(i));
      AddChildView(button);
    }
    Layout();
  }

  WindowListModel* model_;
  ActivateCallback activate_callback_;

  DISALLOW_COPY_AND_ASSIGN(WindowListView);
};

class SimpleWM::FrameView : public views::WidgetDelegateView,
                            public aura::WindowObserver {
 public:
  explicit FrameView(aura::Window* client_window)
      : client_window_(client_window) {
    client_window_->AddObserver(this);
  }
  ~FrameView() override = default;

  void Init() {
    move_event_handler_ =
        std::make_unique<MoveEventHandler>(GetWidget()->GetNativeWindow());
  }

 private:
  // views::WidgetDelegateView:
  base::string16 GetWindowTitle() const override {
    base::string16* title_from_property =
        client_window_->GetProperty(aura::client::kTitleKey);
    base::string16 title = title_from_property ? *title_from_property
                                               : base::UTF8ToUTF16("(Window)");
    // TODO(beng): quick hack to cause WindowObserver::OnWindowTitleChanged to
    //             fire.
    GetWidget()->GetNativeWindow()->SetTitle(title);
    return title;
  }
  void Layout() override {
    // Client offsets are applied automatically by the window service.
    gfx::Rect parent_bounds = GetWidget()->GetNativeWindow()->bounds();
    parent_bounds.set_origin(gfx::Point());

    if (GetWidget()->IsMaximized()) {
      parent_bounds.Inset(-kNonClientSize, -kNonClientMaximizedTopOverlap,
                          -kNonClientSize, -kNonClientSize);
    }

    client_window_->SetBounds(parent_bounds);
  }
  bool CanMaximize() const override {
    return (client_window_->GetProperty(aura::client::kResizeBehaviorKey) &
        ui::mojom::kResizeBehaviorCanMaximize) != 0;
  }

  bool CanMinimize() const override {
    return (client_window_->GetProperty(aura::client::kResizeBehaviorKey) &
        ui::mojom::kResizeBehaviorCanMinimize) != 0;
  }

  bool CanResize() const override {
    return (client_window_->GetProperty(aura::client::kResizeBehaviorKey) &
        ui::mojom::kResizeBehaviorCanResize) != 0;
  }

  // aura::WindowObserver:
  void OnWindowPropertyChanged(aura::Window* window, const void* key,
                               intptr_t old) override {
    if (key == aura::client::kTitleKey)
      GetWidget()->UpdateWindowTitle();
    else if (key == aura::client::kResizeBehaviorKey)
      GetWidget()->non_client_view()->frame_view()->Layout();
  }

  aura::Window* client_window_;
  std::unique_ptr<MoveEventHandler> move_event_handler_;

  DISALLOW_COPY_AND_ASSIGN(FrameView);
};

class SimpleWM::WorkspaceLayoutManager : public aura::WindowObserver {
 public:
  explicit WorkspaceLayoutManager(aura::Window* window_root)
      : window_root_(window_root) {}
  ~WorkspaceLayoutManager() override = default;

 private:
  // aura::WindowObserver:
  void OnWindowPropertyChanged(aura::Window* window, const void* key,
                               intptr_t old) override {
    if (key == aura::client::kShowStateKey) {
      ui::WindowShowState show_state =
          window->GetProperty(aura::client::kShowStateKey);
      switch (show_state) {
      case ui::SHOW_STATE_NORMAL:
        window->Show();
        window->SetBounds(
            *window->GetProperty(aura::client::kRestoreBoundsKey));
        break;
      case ui::SHOW_STATE_MINIMIZED:
        window->Hide();
        break;
      case ui::SHOW_STATE_MAXIMIZED:
        window->Show();
        window->SetProperty(aura::client::kRestoreBoundsKey,
                            new gfx::Rect(window->bounds()));
        window->SetBounds(gfx::Rect(window_root_->bounds().size()));
        break;
      default:
        NOTREACHED();
        break;
      }
    }
  }
  void OnWindowDestroying(aura::Window* window) override {
    window->RemoveObserver(this);
  }

  aura::Window* window_root_;

  DISALLOW_COPY_AND_ASSIGN(WorkspaceLayoutManager);
};

class SimpleWM::DisplayLayoutManager : public aura::LayoutManager {
 public:
  DisplayLayoutManager(aura::Window* display_root,
                       aura::Window* window_root,
                       SimpleWM::WindowListView* window_list_view)
      : display_root_(display_root),
        window_root_(window_root),
        window_list_view_(window_list_view) {}
  ~DisplayLayoutManager() override = default;

 private:
  // aura::LayoutManager:
  void OnWindowResized() override {}
  void OnWindowAddedToLayout(aura::Window* child) override {
    Layout();
  }
  void OnWillRemoveWindowFromLayout(aura::Window* child) override {}
  void OnWindowRemovedFromLayout(aura::Window* child) override {}
  void OnChildWindowVisibilityChanged(aura::Window* child,
                                      bool visible) override {}
  void SetChildBounds(aura::Window* child,
                      const gfx::Rect& requested_bounds) override {
    SetChildBoundsDirect(child, requested_bounds);
  }

  void Layout() {
    gfx::Size ps = window_list_view_->GetPreferredSize();
    gfx::Rect bounds = display_root_->bounds();
    gfx::Rect window_root_bounds = bounds;
    window_root_bounds.set_height(window_root_bounds.height() - ps.height());
    window_root_->SetBounds(window_root_bounds);
    gfx::Rect window_list_view_bounds = bounds;
    window_list_view_bounds.set_height(ps.height());
    window_list_view_bounds.set_y(window_root_bounds.bottom());
    window_list_view_->GetWidget()->SetBounds(window_list_view_bounds);
  }

  aura::Window* display_root_;
  aura::Window* window_root_;
  SimpleWM::WindowListView* window_list_view_;

  DISALLOW_COPY_AND_ASSIGN(DisplayLayoutManager);
};

////////////////////////////////////////////////////////////////////////////////
// SimpleWM, public:

SimpleWM::SimpleWM() = default;

SimpleWM::~SimpleWM() {
  // WindowTreeHost uses state from WindowTreeClient, so destroy it first.
  window_tree_host_.reset();

  // WindowTreeClient destruction may callback to us.
  window_tree_client_.reset();

  display::Screen::SetScreenInstance(nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// SimpleWM, service_manager::Service implementation:

void SimpleWM::OnStart() {
  CHECK(!started_);
  started_ = true;
  screen_ = std::make_unique<display::ScreenBase>();
  display::Screen::SetScreenInstance(screen_.get());
  aura_init_ = views::AuraInit::Create(
      context()->connector(), context()->identity(), "views_mus_resources.pak",
      std::string(), nullptr, views::AuraInit::Mode::AURA_MUS_WINDOW_MANAGER);
  if (!aura_init_) {
    context()->QuitNow();
    return;
  }
  window_tree_client_ = aura::WindowTreeClient::CreateForWindowManager(
      context()->connector(), this, this);
  aura::Env::GetInstance()->SetWindowTreeClient(window_tree_client_.get());
}

////////////////////////////////////////////////////////////////////////////////
// SimpleWM, aura::WindowTreeClientDelegate implementation:

void SimpleWM::OnEmbed(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) {
  // WindowTreeClients configured as the window manager should never get
  // OnEmbed().
  NOTREACHED();
}

void SimpleWM::OnLostConnection(aura::WindowTreeClient* client) {
  window_tree_host_.reset();
  window_tree_client_.reset();
}

void SimpleWM::OnEmbedRootDestroyed(aura::WindowTreeHostMus* window_tree_host) {
  // WindowTreeClients configured as the window manager should never get
  // OnEmbedRootDestroyed().
  NOTREACHED();
}

void SimpleWM::OnPointerEventObserved(const ui::PointerEvent& event,
                                      int64_t display_id,
                                      aura::Window* target) {
  // Don't care.
}

aura::PropertyConverter* SimpleWM::GetPropertyConverter() {
  return &property_converter_;
}

////////////////////////////////////////////////////////////////////////////////
// SimpleWM, aura::WindowManagerDelegate implementation:

void SimpleWM::SetWindowManagerClient(
    aura::WindowManagerClient* client) {
  window_manager_client_ = client;
}

void SimpleWM::OnWmConnected() {}

void SimpleWM::OnWmSetBounds(aura::Window* window, const gfx::Rect& bounds) {
  FrameView* frame_view = GetFrameViewForClientWindow(window);
  frame_view->GetWidget()->SetBounds(bounds);
}

bool SimpleWM::OnWmSetProperty(
    aura::Window* window,
    const std::string& name,
    std::unique_ptr<std::vector<uint8_t>>* new_data) {
  return true;
}

void SimpleWM::OnWmSetModalType(aura::Window* window, ui::ModalType type) {}

void SimpleWM::OnWmSetCanFocus(aura::Window* window, bool can_focus) {}

aura::Window* SimpleWM::OnWmCreateTopLevelWindow(
    ui::mojom::WindowType window_type,
    std::map<std::string, std::vector<uint8_t>>* properties) {
  aura::Window* client_window = new aura::Window(nullptr);
  SetWindowType(client_window, window_type);
  client_window->Init(ui::LAYER_NOT_DRAWN);

  views::Widget* frame_widget = new views::Widget;
  views::NativeWidgetAura* frame_native_widget =
      new views::NativeWidgetAura(frame_widget, true);
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_WINDOW);
  FrameView* frame_view = new FrameView(client_window);
  params.delegate = frame_view;
  params.native_widget = frame_native_widget;
  params.parent = window_root_;
  params.bounds = gfx::Rect(10, 10, 500, 500);
  frame_widget->Init(params);
  frame_widget->Show();
  frame_view->Init();

  frame_widget->GetNativeWindow()->AddChild(client_window);
  frame_widget->GetNativeWindow()->AddObserver(workspace_layout_manager_.get());

  client_window_to_frame_view_[client_window] = frame_view;
  // TODO(beng): probably need to observe client_window from now on so we can
  // clean up this map.

  return client_window;
}

void SimpleWM::OnWmClientJankinessChanged(
    const std::set<aura::Window*>& client_windows,
    bool janky) {
  // Don't care.
}

void SimpleWM::OnWmBuildDragImage(const gfx::Point& screen_location,
                                  const SkBitmap& drag_image,
                                  const gfx::Vector2d& drag_image_offset,
                                  ui::mojom::PointerKind source) {}

void SimpleWM::OnWmMoveDragImage(const gfx::Point& screen_location) {}

void SimpleWM::OnWmDestroyDragImage() {}

void SimpleWM::OnWmWillCreateDisplay(const display::Display& display) {
  screen_->display_list().AddDisplay(display,
                                     display::DisplayList::Type::PRIMARY);
}

void SimpleWM::OnWmNewDisplay(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host,
    const display::Display& display) {
  // Only handles a single root.
  DCHECK(!window_root_);
  window_tree_host_ = std::move(window_tree_host);
  window_tree_host_->InitHost();
  window_tree_host_->window()->Show();
  display_root_ = window_tree_host_->window();
  window_root_ = new aura::Window(nullptr);
  window_root_->Init(ui::LAYER_SOLID_COLOR);
  window_root_->layer()->SetColor(SK_ColorWHITE);
  display_root_->AddChild(window_root_);
  window_root_->Show();
  workspace_layout_manager_ =
      std::make_unique<WorkspaceLayoutManager>(window_root_);

  window_list_model_ = std::make_unique<WindowListModel>(window_root_);

  views::Widget* window_list_widget = new views::Widget;
  views::NativeWidgetAura* window_list_widget_native_widget =
      new views::NativeWidgetAura(window_list_widget, true);
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_CONTROL);
  WindowListView* window_list_view =
      new WindowListView(window_list_model_.get(),
                         base::Bind(&SimpleWM::OnWindowListViewItemActivated,
                                    base::Unretained(this)));
  params.delegate = window_list_view;
  params.native_widget = window_list_widget_native_widget;
  params.parent = display_root_;
  window_list_widget->Init(params);
  window_list_widget->Show();

  display_root_->SetLayoutManager(new DisplayLayoutManager(
      display_root_, window_root_, window_list_view));

  DCHECK(window_manager_client_);
  window_manager_client_->AddActivationParent(window_root_);
  ui::mojom::FrameDecorationValuesPtr frame_decoration_values =
      ui::mojom::FrameDecorationValues::New();
  frame_decoration_values->normal_client_area_insets.Set(
      kNonClientTopHeight, kNonClientSize, kNonClientSize, kNonClientSize);
  frame_decoration_values->max_title_bar_button_width = 0;
  window_manager_client_->SetFrameDecorationValues(
      std::move(frame_decoration_values));
  focus_controller_ = std::make_unique<wm::FocusController>(this);
  aura::client::SetFocusClient(display_root_, focus_controller_.get());
  wm::SetActivationClient(display_root_, focus_controller_.get());
  display_root_->AddPreTargetHandler(focus_controller_.get());
}

void SimpleWM::OnWmDisplayRemoved(
    aura::WindowTreeHostMus* window_tree_host) {
  DCHECK_EQ(window_tree_host, window_tree_host_.get());
  window_root_ = nullptr;
  window_tree_host_.reset();
}

void SimpleWM::OnWmDisplayModified(const display::Display& display) {}

void SimpleWM::OnWmPerformMoveLoop(
    aura::Window* window,
    ui::mojom::MoveLoopSource source,
    const gfx::Point& cursor_location,
    const base::Callback<void(bool)>& on_done) {
  // Don't care.
}

void SimpleWM::OnWmCancelMoveLoop(aura::Window* window) {}

void SimpleWM::OnCursorTouchVisibleChanged(bool enabled) {}

void SimpleWM::OnWmSetClientArea(
    aura::Window* window,
    const gfx::Insets& insets,
    const std::vector<gfx::Rect>& additional_client_areas) {}

bool SimpleWM::IsWindowActive(aura::Window* window) { return false; }

void SimpleWM::OnWmDeactivateWindow(aura::Window* window) {}

void SimpleWM::OnWmPerformAction(aura::Window* window,
                                 const std::string& action) {}

////////////////////////////////////////////////////////////////////////////////
// SimpleWM, wm::BaseFocusRules implementation:

bool SimpleWM::SupportsChildActivation(aura::Window* window) const {
  return window == window_root_;
}

bool SimpleWM::IsWindowConsideredVisibleForActivation(
    aura::Window* window) const {
  if (window->IsVisible())
    return true;

  ui::WindowShowState show_state =
    window->GetProperty(aura::client::kShowStateKey);
  if (show_state == ui::SHOW_STATE_MINIMIZED)
    return true;

  return window->TargetVisibility();
}

////////////////////////////////////////////////////////////////////////////////
// SimpleWM, private:

SimpleWM::FrameView* SimpleWM::GetFrameViewForClientWindow(
    aura::Window* client_window) {
  auto it = client_window_to_frame_view_.find(client_window);
  return it != client_window_to_frame_view_.end() ? it->second : nullptr;
}

void SimpleWM::OnWindowListViewItemActivated(aura::Window* window) {
  window->Show();
  wm::ActivationClient* activation_client =
      wm::GetActivationClient(window->GetRootWindow());
  activation_client->ActivateWindow(window);
}

}  // namespace simple_wm
