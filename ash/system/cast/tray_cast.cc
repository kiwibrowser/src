// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/cast/tray_cast.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "ash/metrics/user_metrics_recorder.h"
#include "ash/public/interfaces/cast_config.mojom.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/screen_security/screen_tray_item.h"
#include "ash/system/tray/hover_highlight_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_item_detailed_view_delegate.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_detailed_view.h"
#include "ash/system/tray/tray_item_more.h"
#include "ash/system/tray/tray_item_view.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/text_elider.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {

namespace {

const size_t kMaximumStatusStringLength = 100;

// Helper method to elide the given string to the maximum length. If a string is
// contains user-input and is displayed, we should elide it.
// TODO(jdufault): This does not properly trim unicode characters. We should
// implement this properly by using views::Label::SetElideBehavior(...). See
// crbug.com/532496.
base::string16 ElideString(const base::string16& text) {
  base::string16 elided;
  gfx::ElideString(text, kMaximumStatusStringLength, &elided);
  return elided;
}

// Returns the correct vector icon for |icon_type|. Some types may be different
// for branded builds.
const gfx::VectorIcon& SinkIconTypeToIcon(mojom::SinkIconType icon_type) {
  switch (icon_type) {
#if defined(GOOGLE_CHROME_BUILD)
    case mojom::SinkIconType::CAST:
      return kSystemMenuCastDeviceIcon;
    case mojom::SinkIconType::EDUCATION:
      return kSystemMenuCastEducationIcon;
    case mojom::SinkIconType::HANGOUT:
      return kSystemMenuCastHangoutIcon;
    case mojom::SinkIconType::MEETING:
      return kSystemMenuCastMeetingIcon;
#else
    case mojom::SinkIconType::CAST:
    case mojom::SinkIconType::EDUCATION:
      return kSystemMenuCastGenericIcon;
    case mojom::SinkIconType::HANGOUT:
    case mojom::SinkIconType::MEETING:
      return kSystemMenuCastMessageIcon;
#endif
    case mojom::SinkIconType::GENERIC:
      return kSystemMenuCastGenericIcon;
    case mojom::SinkIconType::CAST_AUDIO_GROUP:
      return kSystemMenuCastAudioGroupIcon;
    case mojom::SinkIconType::CAST_AUDIO:
      return kSystemMenuCastAudioIcon;
    case mojom::SinkIconType::WIRED_DISPLAY:
      return kSystemMenuCastGenericIcon;
  }

  NOTREACHED();
  return kSystemMenuCastGenericIcon;
}

}  // namespace

namespace tray {

// This view is displayed in the system tray when the cast extension is active.
// It asks the user if they want to cast the desktop. If they click on the
// chevron, then a detail view will replace this view where the user will
// actually pick the cast receiver.
class CastSelectDefaultView : public TrayItemMore {
 public:
  explicit CastSelectDefaultView(SystemTrayItem* owner);
  ~CastSelectDefaultView() override;

 protected:
  // TrayItemMore:
  void UpdateStyle() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastSelectDefaultView);
};

CastSelectDefaultView::CastSelectDefaultView(SystemTrayItem* owner)
    : TrayItemMore(owner) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  base::string16 label =
      rb.GetLocalizedString(IDS_ASH_STATUS_TRAY_CAST_DESKTOP);
  SetLabel(label);
  SetAccessibleName(label);
}

CastSelectDefaultView::~CastSelectDefaultView() = default;

void CastSelectDefaultView::UpdateStyle() {
  TrayItemMore::UpdateStyle();

  std::unique_ptr<TrayPopupItemStyle> style = CreateStyle();
  SetImage(gfx::CreateVectorIcon(kSystemMenuCastIcon, style->GetIconColor()));
}

// This view is displayed when the screen is actively being casted; it allows
// the user to easily stop casting. It fully replaces the
// |CastSelectDefaultView| view inside of the |CastDuplexView|.
class CastCastView : public ScreenStatusView {
 public:
  CastCastView();
  ~CastCastView() override;

  void StopCasting();

  const std::string& displayed_route_id() const { return displayed_route_->id; }

  // Updates the label for the stop view to include information about the
  // current device that is being casted.
  void UpdateLabel(const std::vector<mojom::SinkAndRoutePtr>& sinks_routes);

 private:
  // Overridden from views::ButtonListener.
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // The cast activity id that we are displaying. If the user stops a cast, we
  // send this value to the config delegate so that we stop the right cast.
  mojom::CastRoutePtr displayed_route_;

  DISALLOW_COPY_AND_ASSIGN(CastCastView);
};

CastCastView::CastCastView()
    : ScreenStatusView(
          nullptr,
          l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_CAST_CAST_UNKNOWN),
          l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_CAST_STOP)) {
  icon()->SetImage(
      gfx::CreateVectorIcon(kSystemMenuCastEnabledIcon, kMenuIconColor));
}

CastCastView::~CastCastView() = default;

void CastCastView::StopCasting() {
  Shell::Get()->cast_config()->StopCasting(displayed_route_.Clone());
  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_CAST_STOP_CAST);
}

void CastCastView::UpdateLabel(
    const std::vector<mojom::SinkAndRoutePtr>& sinks_routes) {
  for (auto& i : sinks_routes) {
    const mojom::CastSinkPtr& sink = i->sink;
    const mojom::CastRoutePtr& route = i->route;

    // We only want to display casts that came from this machine, since on a
    // busy network many other people could be casting.
    if (!route->id.empty() && route->is_local_source) {
      displayed_route_ = route.Clone();

      // We want to display different labels inside of the title depending on
      // what we are actually casting - either the desktop, a tab, or a fallback
      // that catches everything else (ie, an extension tab).
      switch (route->content_source) {
        case ash::mojom::ContentSource::UNKNOWN:
          label()->SetText(
              l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_CAST_CAST_UNKNOWN));
          stop_button()->SetAccessibleName(l10n_util::GetStringUTF16(
              IDS_ASH_STATUS_TRAY_CAST_CAST_UNKNOWN_ACCESSIBILITY_STOP));
          break;
        case ash::mojom::ContentSource::TAB:
          label()->SetText(ElideString(l10n_util::GetStringFUTF16(
              IDS_ASH_STATUS_TRAY_CAST_CAST_TAB,
              base::UTF8ToUTF16(route->title), base::UTF8ToUTF16(sink->name))));
          stop_button()->SetAccessibleName(
              ElideString(l10n_util::GetStringFUTF16(
                  IDS_ASH_STATUS_TRAY_CAST_CAST_TAB_ACCESSIBILITY_STOP,
                  base::UTF8ToUTF16(route->title),
                  base::UTF8ToUTF16(sink->name))));
          break;
        case ash::mojom::ContentSource::DESKTOP:
          label()->SetText(ElideString(
              l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_CAST_CAST_DESKTOP,
                                         base::UTF8ToUTF16(sink->name))));
          stop_button()->SetAccessibleName(
              ElideString(l10n_util::GetStringFUTF16(
                  IDS_ASH_STATUS_TRAY_CAST_CAST_DESKTOP_ACCESSIBILITY_STOP,
                  base::UTF8ToUTF16(sink->name))));
          break;
      }

      PreferredSizeChanged();
      Layout();
      // Only need to update labels once.
      break;
    }
  }
}

void CastCastView::ButtonPressed(views::Button* sender,
                                 const ui::Event& event) {
  StopCasting();
}

// This view by itself does very little. It acts as a front-end for managing
// which of the two child views (|CastSelectDefaultView| and |CastCastView|)
// is active.
class CastDuplexView : public views::View {
 public:
  CastDuplexView(SystemTrayItem* owner,
                 bool enabled,
                 const std::vector<mojom::SinkAndRoutePtr>& sinks_routes);
  ~CastDuplexView() override;

  // Activate either the casting or select view.
  void ActivateCastView();
  void ActivateSelectView();

  CastSelectDefaultView* select_view() { return select_view_; }
  CastCastView* cast_view() { return cast_view_; }

 private:
  // Overridden from views::View.
  void ChildPreferredSizeChanged(views::View* child) override;
  void Layout() override;

  // Only one of |select_view_| or |cast_view_| will be displayed at any given
  // time. This will return the view is being displayed.
  views::View* ActiveChildView();

  CastSelectDefaultView* select_view_;
  CastCastView* cast_view_;

  DISALLOW_COPY_AND_ASSIGN(CastDuplexView);
};

CastDuplexView::CastDuplexView(
    SystemTrayItem* owner,
    bool enabled,
    const std::vector<mojom::SinkAndRoutePtr>& sinks_routes) {
  select_view_ = new CastSelectDefaultView(owner);
  select_view_->SetEnabled(enabled);
  cast_view_ = new CastCastView();
  cast_view_->UpdateLabel(sinks_routes);
  SetLayoutManager(std::make_unique<views::FillLayout>());

  ActivateSelectView();
}

CastDuplexView::~CastDuplexView() {
  RemoveChildView(ActiveChildView());
  delete select_view_;
  delete cast_view_;
}

void CastDuplexView::ActivateCastView() {
  if (ActiveChildView() == cast_view_)
    return;

  RemoveChildView(select_view_);
  AddChildView(cast_view_);
  InvalidateLayout();
}

void CastDuplexView::ActivateSelectView() {
  if (ActiveChildView() == select_view_)
    return;

  RemoveChildView(cast_view_);
  AddChildView(select_view_);
  InvalidateLayout();
}

void CastDuplexView::ChildPreferredSizeChanged(views::View* child) {
  PreferredSizeChanged();
}

void CastDuplexView::Layout() {
  views::View::Layout();

  select_view_->SetBoundsRect(GetContentsBounds());
  cast_view_->SetBoundsRect(GetContentsBounds());
}

views::View* CastDuplexView::ActiveChildView() {
  if (cast_view_->parent() == this)
    return cast_view_;
  if (select_view_->parent() == this)
    return select_view_;
  return nullptr;
}

// Exposes an icon in the tray. |TrayCast| manages the visiblity of this.
class CastTrayView : public TrayItemView {
 public:
  explicit CastTrayView(SystemTrayItem* tray_item);
  ~CastTrayView() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastTrayView);
};

CastTrayView::CastTrayView(SystemTrayItem* tray_item)
    : TrayItemView(tray_item) {
  CreateImageView();
  image_view()->SetImage(
      gfx::CreateVectorIcon(kSystemTrayCastIcon, kTrayIconColor));
}

CastTrayView::~CastTrayView() = default;

// This view displays a list of cast receivers that can be clicked on and casted
// to. It is activated by clicking on the chevron inside of
// |CastSelectDefaultView|.
class CastDetailedView : public TrayDetailedView {
 public:
  CastDetailedView(DetailedViewDelegate* delegate,
                   const std::vector<mojom::SinkAndRoutePtr>& sinks_and_routes);
  ~CastDetailedView() override;

  // Makes the detail view think the view associated with the given receiver_id
  // was clicked. This will start a cast.
  void SimulateViewClickedForTest(const std::string& receiver_id);

  // Updates the list of available receivers.
  void UpdateReceiverList(
      const std::vector<mojom::SinkAndRoutePtr>& sinks_routes);

 private:
  void CreateItems();

  void UpdateReceiverListFromCachedData();

  // TrayDetailedView:
  void HandleViewClicked(views::View* view) override;

  // A mapping from the receiver id to the receiver/activity data.
  std::map<std::string, ash::mojom::SinkAndRoutePtr> sinks_and_routes_;
  // A mapping from the view pointer to the associated activity id.
  std::map<views::View*, ash::mojom::CastSinkPtr> view_to_sink_map_;

  DISALLOW_COPY_AND_ASSIGN(CastDetailedView);
};

CastDetailedView::CastDetailedView(
    DetailedViewDelegate* delegate,
    const std::vector<mojom::SinkAndRoutePtr>& sinks_routes)
    : TrayDetailedView(delegate) {
  CreateItems();
  UpdateReceiverList(sinks_routes);
}

CastDetailedView::~CastDetailedView() = default;

void CastDetailedView::SimulateViewClickedForTest(
    const std::string& receiver_id) {
  for (const auto& it : view_to_sink_map_) {
    if (it.second->id == receiver_id) {
      HandleViewClicked(it.first);
      break;
    }
  }
}

void CastDetailedView::CreateItems() {
  CreateScrollableList();
  CreateTitleRow(IDS_ASH_STATUS_TRAY_CAST);
}

void CastDetailedView::UpdateReceiverList(
    const std::vector<mojom::SinkAndRoutePtr>& sinks_routes) {
  // Add/update existing.
  for (const auto& it : sinks_routes)
    sinks_and_routes_[it->sink->id] = it->Clone();

  // Remove non-existent sinks. Removing an element invalidates all existing
  // iterators.
  auto i = sinks_and_routes_.begin();
  while (i != sinks_and_routes_.end()) {
    bool has_receiver = false;
    for (auto& receiver : sinks_routes) {
      if (i->first == receiver->sink->id)
        has_receiver = true;
    }

    if (has_receiver)
      ++i;
    else
      i = sinks_and_routes_.erase(i);
  }

  // Update UI.
  UpdateReceiverListFromCachedData();
  Layout();
}

void CastDetailedView::UpdateReceiverListFromCachedData() {
  // Remove all of the existing views.
  view_to_sink_map_.clear();
  scroll_content()->RemoveAllChildViews(true);

  // Add a view for each receiver.
  for (auto& it : sinks_and_routes_) {
    const ash::mojom::SinkAndRoutePtr& sink_route = it.second;
    const base::string16 name = base::UTF8ToUTF16(sink_route->sink->name);
    views::View* container = AddScrollListItem(
        SinkIconTypeToIcon(sink_route->sink->sink_icon_type), name);
    view_to_sink_map_[container] = sink_route->sink.Clone();
  }

  scroll_content()->SizeToPreferredSize();
  scroller()->Layout();
}

void CastDetailedView::HandleViewClicked(views::View* view) {
  // Find the receiver we are going to cast to.
  auto it = view_to_sink_map_.find(view);
  if (it != view_to_sink_map_.end()) {
    Shell::Get()->cast_config()->CastToSink(it->second.Clone());
    Shell::Get()->metrics()->RecordUserMetricsAction(
        UMA_STATUS_AREA_DETAILED_CAST_VIEW_LAUNCH_CAST);
  }
}

}  // namespace tray

TrayCast::TrayCast(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_CAST),
      detailed_view_delegate_(
          std::make_unique<SystemTrayItemDetailedViewDelegate>(this)) {
  Shell::Get()->AddShellObserver(this);
  Shell::Get()->cast_config()->AddObserver(this);
  Shell::Get()->cast_config()->RequestDeviceRefresh();
}

TrayCast::~TrayCast() {
  Shell::Get()->cast_config()->RemoveObserver(this);
  Shell::Get()->RemoveShellObserver(this);
}

void TrayCast::StartCastForTest(const std::string& receiver_id) {
  if (detailed_ != nullptr)
    detailed_->SimulateViewClickedForTest(receiver_id);
}

void TrayCast::StopCastForTest() {
  default_->cast_view()->StopCasting();
}

const std::string& TrayCast::GetDisplayedCastId() {
  return default_->cast_view()->displayed_route_id();
}

const views::View* TrayCast::GetDefaultView() const {
  return default_;
}

views::View* TrayCast::CreateTrayView(LoginStatus status) {
  CHECK(tray_ == nullptr);
  tray_ = new tray::CastTrayView(this);
  tray_->SetVisible(Shell::Get()->cast_config()->HasActiveRoute());
  return tray_;
}

views::View* TrayCast::CreateDefaultView(LoginStatus status) {
  CHECK(default_ == nullptr);

  default_ = new tray::CastDuplexView(this, status != LoginStatus::LOCKED,
                                      sinks_and_routes_);
  default_->set_id(TRAY_VIEW);
  default_->select_view()->set_id(SELECT_VIEW);
  default_->cast_view()->set_id(CAST_VIEW);

  UpdatePrimaryView();
  return default_;
}

views::View* TrayCast::CreateDetailedView(LoginStatus status) {
  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_DETAILED_CAST_VIEW);
  CHECK(detailed_ == nullptr);
  detailed_ = new tray::CastDetailedView(detailed_view_delegate_.get(),
                                         sinks_and_routes_);
  return detailed_;
}

void TrayCast::OnTrayViewDestroyed() {
  tray_ = nullptr;
}

void TrayCast::OnDefaultViewDestroyed() {
  default_ = nullptr;
}

void TrayCast::OnDetailedViewDestroyed() {
  detailed_ = nullptr;
}

void TrayCast::OnDevicesUpdated(std::vector<mojom::SinkAndRoutePtr> devices) {
  sinks_and_routes_ = std::move(devices);
  UpdatePrimaryView();

  if (default_) {
    bool has_receivers = !sinks_and_routes_.empty();
    default_->SetVisible(has_receivers);
    default_->cast_view()->UpdateLabel(sinks_and_routes_);
  }
  if (detailed_)
    detailed_->UpdateReceiverList(sinks_and_routes_);
}

void TrayCast::UpdatePrimaryView() {
  if (Shell::Get()->cast_config()->Connected() && !sinks_and_routes_.empty()) {
    if (default_) {
      if (Shell::Get()->cast_config()->HasActiveRoute())
        default_->ActivateCastView();
      else
        default_->ActivateSelectView();
    }

    if (tray_)
      tray_->SetVisible(is_mirror_casting_);
  } else {
    if (default_)
      default_->SetVisible(false);
    if (tray_)
      tray_->SetVisible(false);
  }
}

void TrayCast::OnCastingSessionStartedOrStopped(bool started) {
  is_mirror_casting_ = started;
  UpdatePrimaryView();
}

}  // namespace ash
