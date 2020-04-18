// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_widget.h"

#include "ash/animation/animation_change_type.h"
#include "ash/focus_cycler.h"
#include "ash/public/cpp/ash_features.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/app_list_button.h"
#include "ash/shelf/login_shelf_view.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_background_animator_observer.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_layout_manager.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shell.h"
#include "ash/system/status_area_layout_manager.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/tray/system_tray.h"
#include "base/command_line.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/views/accessible_pane_view.h"
#include "ui/views/focus/focus_search.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace ash {
namespace {

// Return the first or last focusable child of |root|.
views::View* FindFirstOrLastFocusableChild(views::View* root,
                                           bool find_last_child) {
  views::FocusSearch search(root, find_last_child /*cycle*/,
                            false /*accessibility_mode*/);
  views::FocusTraversable* dummy_focus_traversable;
  views::View* dummy_focus_traversable_view;
  return search.FindNextFocusableView(
      root,
      find_last_child ? views::FocusSearch::SearchDirection::kBackwards
                      : views::FocusSearch::SearchDirection::kForwards,
      views::FocusSearch::TraversalDirection::kDown,
      views::FocusSearch::StartingViewPolicy::kSkipStartingView,
      views::FocusSearch::AnchoredDialogPolicy::kCanGoIntoAnchoredDialog,
      &dummy_focus_traversable, &dummy_focus_traversable_view);
}

}  // namespace

// The contents view of the Shelf. This view contains ShelfView and
// sizes it to the width of the shelf minus the size of the status area.
class ShelfWidget::DelegateView : public views::WidgetDelegate,
                                  public views::AccessiblePaneView,
                                  public ShelfBackgroundAnimatorObserver {
 public:
  explicit DelegateView(ShelfWidget* shelf);
  ~DelegateView() override;

  void set_focus_cycler(FocusCycler* focus_cycler) {
    focus_cycler_ = focus_cycler;
  }

  FocusCycler* focus_cycler() { return focus_cycler_; }

  ui::Layer* opaque_background() { return &opaque_background_; }

  void SetParentLayer(ui::Layer* layer);

  void set_default_last_focusable_child(bool default_last_focusable_child) {
    default_last_focusable_child_ = default_last_focusable_child;
  }

  // views::WidgetDelegate:
  void DeleteDelegate() override { delete this; }
  views::Widget* GetWidget() override { return View::GetWidget(); }
  const views::Widget* GetWidget() const override { return View::GetWidget(); }

  bool CanActivate() const override;
  void ReorderChildLayers(ui::Layer* parent_layer) override;
  // This will be called when the parent local bounds change.
  void OnBoundsChanged(const gfx::Rect& old_bounds) override;

  // views::AccessiblePaneView:
  views::View* GetDefaultFocusableChild() override;

  // ShelfBackgroundAnimatorObserver:
  void UpdateShelfBackground(SkColor color) override;

 private:
  ShelfWidget* shelf_widget_;
  FocusCycler* focus_cycler_;
  // A background layer that may be visible depending on a
  // ShelfBackgroundAnimator.
  ui::Layer opaque_background_;

  // When true, the default focus of the shelf is the last focusable child.
  bool default_last_focusable_child_ = false;

  DISALLOW_COPY_AND_ASSIGN(DelegateView);
};

ShelfWidget::DelegateView::DelegateView(ShelfWidget* shelf_widget)
    : shelf_widget_(shelf_widget),
      focus_cycler_(nullptr),
      opaque_background_(ui::LAYER_SOLID_COLOR) {
  DCHECK(shelf_widget_);
  set_owned_by_client();  // Deleted by DeleteDelegate().

  SetLayoutManager(std::make_unique<views::FillLayout>());
  set_allow_deactivate_on_esc(true);
  opaque_background_.SetBounds(GetLocalBounds());
}

ShelfWidget::DelegateView::~DelegateView() = default;

// static
bool ShelfWidget::IsUsingViewsShelf() {
  switch (Shell::Get()->session_controller()->GetSessionState()) {
    case session_manager::SessionState::ACTIVE:
      return true;
    // See https://crbug.com/798869.
    case session_manager::SessionState::OOBE:
      return false;
    case session_manager::SessionState::LOCKED:
    case session_manager::SessionState::LOGIN_SECONDARY:
      return switches::IsUsingViewsLock();
    case session_manager::SessionState::UNKNOWN:
    case session_manager::SessionState::LOGIN_PRIMARY:
    case session_manager::SessionState::LOGGED_IN_NOT_ACTIVE:
      return features::IsViewsLoginEnabled();
  }
}

void ShelfWidget::DelegateView::SetParentLayer(ui::Layer* layer) {
  layer->Add(&opaque_background_);
  ReorderLayers();
}

bool ShelfWidget::DelegateView::CanActivate() const {
  // Only allow activation from the focus cycler, not from mouse events, etc.
  return focus_cycler_ && focus_cycler_->widget_activating() == GetWidget();
}

void ShelfWidget::DelegateView::ReorderChildLayers(ui::Layer* parent_layer) {
  views::View::ReorderChildLayers(parent_layer);
  parent_layer->StackAtBottom(&opaque_background_);
}

void ShelfWidget::DelegateView::OnBoundsChanged(const gfx::Rect& old_bounds) {
  opaque_background_.SetBounds(GetLocalBounds());
}

views::View* ShelfWidget::DelegateView::GetDefaultFocusableChild() {
  // If views-based login shelf is shown, we want to focus either its first or
  // last child, otherwise focus on the first child as default.
  if (IsUsingViewsShelf())
    return FindFirstOrLastFocusableChild(shelf_widget_->login_shelf_view_,
                                         default_last_focusable_child_);
  return GetFirstFocusableChild();
}

void ShelfWidget::DelegateView::UpdateShelfBackground(SkColor color) {
  opaque_background_.SetColor(color);
}

ShelfWidget::ShelfWidget(aura::Window* shelf_container, Shelf* shelf)
    : shelf_(shelf),
      delegate_view_(new DelegateView(this)),
      shelf_view_(new ShelfView(Shell::Get()->shelf_model(), shelf_, this)),
      login_shelf_view_(
          new LoginShelfView(RootWindowController::ForWindow(shelf_container)
                                 ->lock_screen_action_background_controller())),
      background_animator_(SHELF_BACKGROUND_DEFAULT,
                           shelf_,
                           Shell::Get()->wallpaper_controller()),
      scoped_session_observer_(this) {
  DCHECK(shelf_container);
  DCHECK(shelf_);

  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.name = "ShelfWidget";
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.delegate = delegate_view_;
  params.parent = shelf_container;
  Init(params);

  // The shelf should not take focus when initially shown.
  set_focus_on_creation(false);
  SetContentsView(delegate_view_);
  delegate_view_->SetParentLayer(GetLayer());

  // The shelf view observes the shelf model and creates icons as items are
  // added to the model.
  shelf_view_->Init();
  GetContentsView()->AddChildView(shelf_view_);
  GetContentsView()->AddChildView(login_shelf_view_);

  shelf_layout_manager_ = new ShelfLayoutManager(this, shelf_);
  shelf_layout_manager_->AddObserver(this);
  shelf_container->SetLayoutManager(shelf_layout_manager_);
  background_animator_.PaintBackground(
      shelf_layout_manager_->GetShelfBackgroundType(),
      AnimationChangeType::IMMEDIATE);

  views::Widget::AddObserver(this);

  // Calls back into |this| and depends on |shelf_view_|.
  background_animator_.AddObserver(this);
  background_animator_.AddObserver(delegate_view_);
}

ShelfWidget::~ShelfWidget() {
  // Must call Shutdown() before destruction.
  DCHECK(!status_area_widget_);
}

void ShelfWidget::Initialize() {
  // Sets initial session state to make sure the UI is properly shown.
  OnSessionStateChanged(Shell::Get()->session_controller()->GetSessionState());
}

void ShelfWidget::Shutdown() {
  // Shutting down the status area widget may cause some widgets (e.g. bubbles)
  // to close, so uninstall the ShelfLayoutManager event filters first. Don't
  // reset the pointer until later because other widgets (e.g. app list) may
  // access it later in shutdown.
  shelf_layout_manager_->PrepareForShutdown();

  background_animator_.RemoveObserver(status_area_widget_.get());
  Shell::Get()->focus_cycler()->RemoveWidget(status_area_widget_.get());
  status_area_widget_.reset();

  // Don't need to update the shelf background during shutdown.
  background_animator_.RemoveObserver(delegate_view_);
  background_animator_.RemoveObserver(this);

  // Don't need to observe focus/activation during shutdown.
  Shell::Get()->focus_cycler()->RemoveWidget(this);
  SetFocusCycler(nullptr);
  RemoveObserver(this);
}

void ShelfWidget::CreateStatusAreaWidget(aura::Window* status_container) {
  DCHECK(status_container);
  DCHECK(!status_area_widget_);
  status_area_widget_ =
      std::make_unique<StatusAreaWidget>(status_container, shelf_);
  status_area_widget_->Initialize();
  Shell::Get()->focus_cycler()->AddWidget(status_area_widget_.get());
  background_animator_.AddObserver(status_area_widget_.get());
  status_container->SetLayoutManager(new StatusAreaLayoutManager(this));
}

void ShelfWidget::SetPaintsBackground(ShelfBackgroundType background_type,
                                      AnimationChangeType change_type) {
  background_animator_.PaintBackground(background_type, change_type);
}

ShelfBackgroundType ShelfWidget::GetBackgroundType() const {
  return background_animator_.target_background_type();
}

int ShelfWidget::GetBackgroundAlphaValue(
    ShelfBackgroundType background_type) const {
  return background_animator_.GetBackgroundAlphaValue(background_type);
}

void ShelfWidget::OnShelfAlignmentChanged() {
  // Check added for http://crbug.com/738011.
  CHECK(status_area_widget_);
  shelf_view_->OnShelfAlignmentChanged();
  status_area_widget_->UpdateAfterShelfAlignmentChange();
  delegate_view_->SchedulePaint();
}

void ShelfWidget::PostCreateShelf() {
  SetFocusCycler(Shell::Get()->focus_cycler());

  // Ensure the newly created |shelf_| gets current values.
  background_animator_.NotifyObserver(this);

  shelf_layout_manager_->LayoutShelf();
  shelf_layout_manager_->UpdateAutoHideState();
  ShowIfHidden();
}

bool ShelfWidget::IsShowingAppList() const {
  return GetAppListButton() && GetAppListButton()->is_showing_app_list();
}

bool ShelfWidget::IsShowingContextMenu() const {
  return shelf_view_->IsShowingMenu();
}

bool ShelfWidget::IsShowingOverflowBubble() const {
  return shelf_view_->IsShowingOverflowBubble();
}

void ShelfWidget::SetFocusCycler(FocusCycler* focus_cycler) {
  delegate_view_->set_focus_cycler(focus_cycler);
  if (focus_cycler)
    focus_cycler->AddWidget(this);
}

FocusCycler* ShelfWidget::GetFocusCycler() {
  return delegate_view_->focus_cycler();
}

void ShelfWidget::UpdateIconPositionForPanel(aura::Window* panel) {
  ShelfID id = ShelfID::Deserialize(panel->GetProperty(kShelfIDKey));
  if (id.IsNull())
    return;

  aura::Window* shelf_window = this->GetNativeWindow();
  gfx::Rect bounds = panel->GetBoundsInScreen();
  ::wm::ConvertRectFromScreen(shelf_window, &bounds);
  shelf_view_->UpdatePanelIconPosition(id, bounds.CenterPoint());
}

gfx::Rect ShelfWidget::GetScreenBoundsOfItemIconForWindow(
    aura::Window* window) {
  ShelfID id = ShelfID::Deserialize(window->GetProperty(kShelfIDKey));
  if (id.IsNull())
    return gfx::Rect();

  gfx::Rect bounds(shelf_view_->GetIdealBoundsOfItemIcon(id));
  gfx::Point screen_origin;
  views::View::ConvertPointToScreen(shelf_view_, &screen_origin);
  return gfx::Rect(screen_origin.x() + bounds.x(),
                   screen_origin.y() + bounds.y(), bounds.width(),
                   bounds.height());
}

AppListButton* ShelfWidget::GetAppListButton() const {
  return shelf_view_->GetAppListButton();
}

BackButton* ShelfWidget::GetBackButton() const {
  return shelf_view_->GetBackButton();
}

app_list::ApplicationDragAndDropHost*
ShelfWidget::GetDragAndDropHostForAppList() {
  return shelf_view_;
}

void ShelfWidget::set_default_last_focusable_child(
    bool default_last_focusable_child) {
  delegate_view_->set_default_last_focusable_child(
      default_last_focusable_child);
}

void ShelfWidget::OnWidgetActivationChanged(views::Widget* widget,
                                            bool active) {
  if (active)
    delegate_view_->SetPaneFocusAndFocusDefault();
  else
    delegate_view_->GetFocusManager()->ClearFocus();
}

void ShelfWidget::UpdateShelfItemBackground(SkColor color) {
  shelf_view_->UpdateShelfItemBackground(color);
}

void ShelfWidget::WillDeleteShelfLayoutManager() {
  shelf_layout_manager_->RemoveObserver(this);
  shelf_layout_manager_ = nullptr;
}

void ShelfWidget::OnSessionStateChanged(session_manager::SessionState state) {
  // Do not show shelf widget:
  // * when views based shelf is disabled
  // * in UNKNOWN state - it might be called before shelf was initialized
  // * on secondary screens in states other than ACTIVE
  bool using_views_shelf = IsUsingViewsShelf();
  bool unknown_state = state == session_manager::SessionState::UNKNOWN;
  bool hide_on_secondary_screen = shelf_->ShouldHideOnSecondaryDisplay(state);
  if (!using_views_shelf || unknown_state || hide_on_secondary_screen) {
    HideIfShown();
  } else {
    switch (state) {
      case session_manager::SessionState::ACTIVE:
        login_shelf_view_->SetVisible(false);
        shelf_view_->SetVisible(true);
        break;
      case session_manager::SessionState::LOCKED:
      case session_manager::SessionState::LOGIN_SECONDARY:
        shelf_view_->SetVisible(false);
        login_shelf_view_->SetVisible(true);
        break;
      case session_manager::SessionState::OOBE:
        login_shelf_view_->SetVisible(true);
        shelf_view_->SetVisible(false);
        break;
      case session_manager::SessionState::LOGIN_PRIMARY:
      case session_manager::SessionState::LOGGED_IN_NOT_ACTIVE:
        login_shelf_view_->SetVisible(true);
        shelf_view_->SetVisible(false);
        break;
      default:
        // session_manager::SessionState::UNKNOWN handled in if statement above.
        NOTREACHED();
    }
    ShowIfHidden();
  }
  login_shelf_view_->UpdateAfterSessionStateChange(state);
}

void ShelfWidget::HideIfShown() {
  if (IsVisible())
    Hide();
}

void ShelfWidget::ShowIfHidden() {
  if (!IsVisible())
    Show();
}

}  // namespace ash
