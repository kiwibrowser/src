// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/search_result_tile_item_view.h"

#include <utility>

#include "ash/app_list/model/app_list_view_state.h"
#include "ash/app_list/model/search/search_result.h"
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/cpp/app_list/app_list_features.h"
#include "ash/public/cpp/app_list/vector_icons/vector_icons.h"
#include "base/bind.h"
#include "base/i18n/number_formatting.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/app_list/app_list_metrics.h"
#include "ui/app_list/app_list_view_delegate.h"
#include "ui/app_list/pagination_model.h"
#include "ui/app_list/views/search_result_container_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_features.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/focus/focus_manager.h"

namespace app_list {

namespace {

constexpr int kSearchTileWidth = 80;
constexpr int kSearchTileTopPadding = 4;
constexpr int kSearchTitleSpacing = 5;
constexpr int kSearchPriceSize = 37;
constexpr int kSearchRatingSize = 26;
constexpr int kSearchRatingStarSize = 12;
constexpr int kSearchRatingStarHorizontalSpacing = 1;
constexpr int kSearchRatingStarVerticalSpacing = 2;

constexpr int kIconSelectedSize = 56;
constexpr int kIconSelectedCornerRadius = 4;
// Icon selected color, #000 8%.
constexpr int kIconSelectedColor = SkColorSetARGB(0x14, 0x00, 0x00, 0x00);

constexpr SkColor kSearchTitleColor = SkColorSetARGB(0xDF, 0x00, 0x00, 0x00);
constexpr SkColor kSearchAppRatingColor =
    SkColorSetARGB(0x8F, 0x00, 0x00, 0x00);
constexpr SkColor kSearchAppPriceColor = SkColorSetARGB(0xFF, 0x0F, 0x9D, 0x58);
constexpr SkColor kSearchRatingStarColor =
    SkColorSetARGB(0x8F, 0x00, 0x00, 0x00);

// The background image source for badge.
class BadgeBackgroundImageSource : public gfx::CanvasImageSource {
 public:
  explicit BadgeBackgroundImageSource(int size)
      : CanvasImageSource(gfx::Size(size, size), false),
        radius_(static_cast<float>(size / 2)) {}
  ~BadgeBackgroundImageSource() override = default;

 private:
  // gfx::CanvasImageSource overrides:
  void Draw(gfx::Canvas* canvas) override {
    cc::PaintFlags flags;
    flags.setColor(SK_ColorWHITE);
    flags.setAntiAlias(true);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawCircle(gfx::PointF(radius_, radius_), radius_, flags);
  }

  const float radius_;

  DISALLOW_COPY_AND_ASSIGN(BadgeBackgroundImageSource);
};

}  // namespace

SearchResultTileItemView::SearchResultTileItemView(
    SearchResultContainerView* result_container,
    AppListViewDelegate* view_delegate,
    PaginationModel* pagination_model)
    : result_container_(result_container),
      view_delegate_(view_delegate),
      pagination_model_(pagination_model),
      is_play_store_app_search_enabled_(
          features::IsPlayStoreAppSearchEnabled()),
      context_menu_(this),
      weak_ptr_factory_(this) {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // When |item_| is null, the tile is invisible. Calling SetSearchResult with a
  // non-null item makes the tile visible.
  SetVisible(false);

  // Prevent the icon view from interfering with our mouse events.
  icon_ = new views::ImageView;
  icon_->set_can_process_events_within_subtree(false);
  icon_->SetVerticalAlignment(views::ImageView::LEADING);
  AddChildView(icon_);

  if (is_play_store_app_search_enabled_) {
    badge_ = new views::ImageView;
    badge_->set_can_process_events_within_subtree(false);
    badge_->SetVerticalAlignment(views::ImageView::LEADING);
    badge_->SetVisible(false);
    AddChildView(badge_);
  }

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  title_ = new views::Label;
  title_->SetAutoColorReadabilityEnabled(false);
  title_->SetEnabledColor(kGridTitleColor);
  title_->SetFontList(rb.GetFontList(kItemTextFontStyle));
  title_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  title_->SetHandlesTooltips(false);
  AddChildView(title_);

  if (is_play_store_app_search_enabled_) {
    const gfx::FontList& font = AppListAppTitleFont();
    rating_ = new views::Label;
    rating_->SetEnabledColor(kSearchAppRatingColor);
    rating_->SetFontList(font);
    rating_->SetLineHeight(font.GetHeight());
    rating_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
    rating_->SetVisible(false);
    AddChildView(rating_);

    rating_star_ = new views::ImageView;
    rating_star_->set_can_process_events_within_subtree(false);
    rating_star_->SetVerticalAlignment(views::ImageView::LEADING);
    rating_star_->SetImage(gfx::CreateVectorIcon(
        kIcBadgeRatingIcon, kSearchRatingStarSize, kSearchRatingStarColor));
    rating_star_->SetVisible(false);
    AddChildView(rating_star_);

    price_ = new views::Label;
    price_->SetEnabledColor(kSearchAppPriceColor);
    price_->SetFontList(font);
    price_->SetLineHeight(font.GetHeight());
    price_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    price_->SetVisible(false);
    AddChildView(price_);
  }

  set_context_menu_controller(this);
}

SearchResultTileItemView::~SearchResultTileItemView() {
  if (item_)
    item_->RemoveObserver(this);
}

void SearchResultTileItemView::SetSearchResult(SearchResult* item) {
  // Handle the case where this may be called from a nested run loop while its
  // context menu is showing. This cancels the menu (it's for the old item).
  context_menu_.Reset();

  SetVisible(!!item);

  SearchResult* old_item = item_;
  if (old_item)
    old_item->RemoveObserver(this);

  item_ = item;

  if (!item)
    return;

  item_->AddObserver(this);

  SetTitle(item_->title());
  SetRating(item_->rating());
  SetPrice(item_->formatted_price());

  const gfx::FontList& font = AppListAppTitleFont();
  if (IsSuggestedAppTile()) {
    title_->SetFontList(font);
    title_->SetLineHeight(font.GetHeight());
    title_->SetEnabledColor(kGridTitleColor);
  } else {
    DCHECK_EQ(ash::SearchResultDisplayType::kTile, item_->display_type());
    // Set solid color background to avoid broken text. See crbug.com/746563.
    if (rating_) {
      rating_->SetBackground(
          views::CreateSolidBackground(kCardBackgroundColor));
    }
    if (price_) {
      price_->SetBackground(views::CreateSolidBackground(kCardBackgroundColor));
    }
    title_->SetBackground(views::CreateSolidBackground(kCardBackgroundColor));
    title_->SetFontList(font);
    title_->SetLineHeight(font.GetHeight());
    title_->SetEnabledColor(kSearchTitleColor);
  }

  title_->SetMaxLines(2);
  title_->SetMultiLine(
      item_->display_type() == ash::SearchResultDisplayType::kTile &&
      item_->result_type() == ash::SearchResultType::kInstalledApp);

  // If the new icon is null, it's being decoded asynchronously. Not updating it
  // now to prevent flickering from showing an empty icon while decoding.
  if (!item->icon().isNull())
    OnMetadataChanged();

  base::string16 accessible_name = title_->text();
  if (rating_ && rating_->visible()) {
    accessible_name +=
        base::UTF8ToUTF16(", ") +
        l10n_util::GetStringFUTF16(IDS_APP_ACCESSIBILITY_STAR_RATING_ARC,
                                   rating_->text());
  }
  if (price_ && price_->visible())
    accessible_name += base::UTF8ToUTF16(", ") + price_->text();
  SetAccessibleName(accessible_name);
}

void SearchResultTileItemView::SetParentBackgroundColor(SkColor color) {
  parent_background_color_ = color;
  UpdateBackgroundColor();
}

void SearchResultTileItemView::OnContextMenuClosed(
    const base::TimeTicks& open_time) {
  base::TimeDelta user_journey_time = base::TimeTicks::Now() - open_time;
  if (IsSuggestedAppTile()) {
    if (view_delegate_->GetModel()->state_fullscreen() ==
        AppListViewState::PEEKING) {
      UMA_HISTOGRAM_TIMES("Apps.ContextMenuUserJourneyTime.SuggestedAppPeeking",
                          user_journey_time);
    } else {
      UMA_HISTOGRAM_TIMES(
          "Apps.ContextMenuUserJourneyTime.SuggestedAppFullscreen",
          user_journey_time);
    }
  } else {
    UMA_HISTOGRAM_TIMES("Apps.ContextMenuUserJourneyTime.SearchResult",
                        user_journey_time);
  }
}

void SearchResultTileItemView::ButtonPressed(views::Button* sender,
                                             const ui::Event& event) {
  if (IsSuggestedAppTile())
    LogAppLaunch();

  RecordSearchResultOpenSource(item_, view_delegate_->GetModel(),
                               view_delegate_->GetSearchModel());
  view_delegate_->OpenSearchResult(item_->id(), event.flags());
}

void SearchResultTileItemView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::Button::GetAccessibleNodeData(node_data);
  // Specify |ax::mojom::StringAttribute::kDescription| with an empty string, so
  // that long truncated names are not read twice. Details of this issue: - The
  // Play Store app's name is shown in a label |title_|. - If the name is too
  // long, it'll get truncated and the full name will
  //   go to the label's tooltip.
  // - SearchResultTileItemView uses that label's tooltip as its tooltip.
  // - If a view doesn't have |ax::mojom::StringAttribute::kDescription| defined
  // in the
  //   |AXNodeData|, |AXViewObjWrapper::Serialize| will use the tooltip text
  //   as its description.
  // - We're customizing this view's accessible name, so it get focused
  //   ChromeVox will read its accessible name and then its description.
  node_data->AddStringAttribute(ax::mojom::StringAttribute::kDescription, "");
}

bool SearchResultTileItemView::OnKeyPressed(const ui::KeyEvent& event) {
  // Return early if |item_| was deleted due to the search result list changing.
  // see crbug.com/801142
  if (!item_)
    return true;

  if (event.key_code() == ui::VKEY_RETURN) {
    if (IsSuggestedAppTile())
      LogAppLaunch();

    RecordSearchResultOpenSource(item_, view_delegate_->GetModel(),
                                 view_delegate_->GetSearchModel());
    view_delegate_->OpenSearchResult(item_->id(), event.flags());
    return true;
  }

  return false;
}

void SearchResultTileItemView::OnFocus() {
  if (pagination_model_ && IsSuggestedAppTile() &&
      view_delegate_->GetModel()->state() == ash::AppListState::kStateApps) {
    // Go back to first page when app in suggestions container is focused.
    pagination_model_->SelectPage(0, false);
  } else if (!IsSuggestedAppTile()) {
    ScrollRectToVisible(GetLocalBounds());
  }
  SetBackgroundHighlighted(true);
  UpdateBackgroundColor();
  NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);
}

void SearchResultTileItemView::OnBlur() {
  SetBackgroundHighlighted(false);
  UpdateBackgroundColor();
}

void SearchResultTileItemView::StateChanged(ButtonState old_state) {
  UpdateBackgroundColor();
}

void SearchResultTileItemView::PaintButtonContents(gfx::Canvas* canvas) {
  if (!item_ || !background_highlighted())
    return;

  gfx::Rect rect(GetContentsBounds());
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  if (IsSuggestedAppTile()) {
    rect.ClampToCenteredSize(gfx::Size(kGridSelectedSize, kGridSelectedSize));
    flags.setColor(kGridSelectedColor);
    canvas->DrawRoundRect(gfx::RectF(rect), kGridSelectedCornerRadius, flags);
  } else {
    DCHECK(item_->display_type() == ash::SearchResultDisplayType::kTile);
    const int kLeftRightPadding = (rect.width() - kIconSelectedSize) / 2;
    rect.Inset(kLeftRightPadding, 0);
    rect.set_height(kIconSelectedSize);
    flags.setColor(kIconSelectedColor);
    canvas->DrawRoundRect(gfx::RectF(rect), kIconSelectedCornerRadius, flags);
  }
}

void SearchResultTileItemView::OnMetadataChanged() {
  SetIcon(item_->icon());
  SetBadgeIcon(item_->badge_icon());
  SetRating(item_->rating());
  SetPrice(item_->formatted_price());
  Layout();
}

void SearchResultTileItemView::OnResultDestroying() {
  // The menu comes from |item_|. If we're showing a menu we need to cancel it.
  context_menu_.Reset();

  if (item_)
    item_->RemoveObserver(this);

  SetSearchResult(nullptr);
}

void SearchResultTileItemView::ShowContextMenuForView(
    views::View* source,
    const gfx::Point& point,
    ui::MenuSourceType source_type) {
  // |item_| could be null when result list is changing.
  if (!item_)
    return;

  view_delegate_->GetSearchResultContextMenuModel(
      item_->id(),
      base::BindOnce(&SearchResultTileItemView::OnGetContextMenuModel,
                     weak_ptr_factory_.GetWeakPtr(), source, point,
                     source_type));
}

void SearchResultTileItemView::OnGetContextMenuModel(
    views::View* source,
    const gfx::Point& point,
    ui::MenuSourceType source_type,
    std::vector<ash::mojom::MenuItemPtr> menu) {
  if (menu.empty() || context_menu_.IsRunning())
    return;

  if (IsSuggestedAppTile()) {
    if (view_delegate_->GetModel()->state_fullscreen() ==
        AppListViewState::PEEKING) {
      UMA_HISTOGRAM_ENUMERATION(
          "Apps.ContextMenuShowSource.SuggestedAppPeeking", source_type,
          ui::MenuSourceType::MENU_SOURCE_TYPE_LAST);
    } else {
      UMA_HISTOGRAM_ENUMERATION(
          "Apps.ContextMenuShowSource.SuggestedAppFullscreen", source_type,
          ui::MenuSourceType::MENU_SOURCE_TYPE_LAST);
    }
  } else {
    UMA_HISTOGRAM_ENUMERATION("Apps.ContextMenuShowSource.SearchResult",
                              source_type,
                              ui::MenuSourceType::MENU_SOURCE_TYPE_LAST);
  }

  // TODO(warx): This is broken (https://crbug.com/795994).
  if (!HasFocus())
    result_container_->ClearSelectedIndex();

  int run_types = views::MenuRunner::HAS_MNEMONICS;
  views::MenuAnchorPosition anchor_type = views::MENU_ANCHOR_TOPLEFT;
  gfx::Rect anchor_rect = gfx::Rect(point, gfx::Size());

  if (::features::IsTouchableAppContextMenuEnabled()) {
    anchor_type = views::MENU_ANCHOR_BUBBLE_TOUCHABLE_LEFT;
    run_types |= views::MenuRunner::USE_TOUCHABLE_LAYOUT |
                 views::MenuRunner::CONTEXT_MENU |
                 views::MenuRunner::FIXED_ANCHOR;
    if (source_type == ui::MenuSourceType::MENU_SOURCE_TOUCH) {
      anchor_rect = source->GetBoundsInScreen();
      // Anchor the menu to the same rect that is used for selection highlight.
      anchor_rect.ClampToCenteredSize(
          gfx::Size(kGridSelectedSize, kGridSelectedSize));
    }
  }
  context_menu_.Build(
      std::move(menu), run_types,
      base::Bind(&SearchResultTileItemView::OnContextMenuClosed,
                 weak_ptr_factory_.GetWeakPtr(), base::TimeTicks::Now()));
  context_menu_.Run(GetWidget(), nullptr, anchor_rect, anchor_type,
                    source_type);

  source->RequestFocus();
}

void SearchResultTileItemView::ExecuteCommand(int command_id, int event_flags) {
  if (item_) {
    view_delegate_->SearchResultContextMenuItemSelected(item_->id(), command_id,
                                                        event_flags);
  }
}

void SearchResultTileItemView::SetIcon(const gfx::ImageSkia& icon) {
  gfx::ImageSkia resized(gfx::ImageSkiaOperations::CreateResizedImage(
      icon, skia::ImageOperations::RESIZE_BEST,
      gfx::Size(kTileIconSize, kTileIconSize)));
  icon_->SetImage(resized);
}

void SearchResultTileItemView::SetBadgeIcon(const gfx::ImageSkia& badge_icon) {
  if (!badge_)
    return;

  if (badge_icon.isNull()) {
    badge_->SetVisible(false);
    return;
  }

  const int size = kBadgeBackgroundRadius * 2;
  gfx::ImageSkia background(std::make_unique<BadgeBackgroundImageSource>(size),
                            gfx::Size(size, size));
  gfx::ImageSkia icon_with_background =
      gfx::ImageSkiaOperations::CreateSuperimposedImage(background, badge_icon);

  gfx::ShadowValues shadow_values;
  shadow_values.push_back(
      gfx::ShadowValue(gfx::Vector2d(0, 1), 0, SkColorSetARGB(0x33, 0, 0, 0)));
  shadow_values.push_back(
      gfx::ShadowValue(gfx::Vector2d(0, 1), 2, SkColorSetARGB(0x33, 0, 0, 0)));
  badge_->SetImage(gfx::ImageSkiaOperations::CreateImageWithDropShadow(
      icon_with_background, shadow_values));
  badge_->SetVisible(true);
}

void SearchResultTileItemView::SetTitle(const base::string16& title) {
  title_->SetText(title);
}

void SearchResultTileItemView::SetRating(float rating) {
  if (!rating_)
    return;

  if (rating < 0) {
    rating_->SetVisible(false);
    rating_star_->SetVisible(false);
    return;
  }

  rating_->SetText(base::FormatDouble(rating, 1));
  rating_->SetVisible(true);
  rating_star_->SetVisible(true);
}

void SearchResultTileItemView::SetPrice(const base::string16& price) {
  if (!price_)
    return;

  if (price.empty()) {
    price_->SetVisible(false);
    return;
  }

  price_->SetText(price);
  price_->SetVisible(true);
}

bool SearchResultTileItemView::IsSuggestedAppTile() const {
  return item_ &&
         item_->display_type() == ash::SearchResultDisplayType::kRecommendation;
}

void SearchResultTileItemView::LogAppLaunch() const {
  // Only log the app launch if the class is being used as a suggested app.
  if (!IsSuggestedAppTile())
    return;

  UMA_HISTOGRAM_BOOLEAN(kAppListAppLaunchedFullscreen,
                        true /* suggested app */);
  base::RecordAction(base::UserMetricsAction("AppList_OpenSuggestedApp"));
}

void SearchResultTileItemView::UpdateBackgroundColor() {
  // Tell the label what color it will be drawn onto. It will use whether the
  // background color is opaque or transparent to decide whether to use subpixel
  // rendering. Does not actually set the label's background color.
  title_->SetBackgroundColor(parent_background_color_);
  SchedulePaint();
}

void SearchResultTileItemView::Layout() {
  gfx::Rect rect(GetContentsBounds());
  if (rect.IsEmpty() || !item_)
    return;

  if (IsSuggestedAppTile()) {
    rect.Inset(0, kGridIconTopPadding, 0, 0);
    icon_->SetBoundsRect(rect);

    rect.Inset(kGridTitleHorizontalPadding,
               kGridIconDimension + kGridTitleSpacing,
               kGridTitleHorizontalPadding, 0);
    rect.set_height(title_->GetPreferredSize().height());
    title_->SetBoundsRect(rect);
  } else {
    DCHECK(item_->display_type() == ash::SearchResultDisplayType::kTile);
    rect.Inset(0, kSearchTileTopPadding, 0, 0);
    icon_->SetBoundsRect(rect);

    if (badge_) {
      gfx::Rect badge_rect(rect);
      gfx::Size icon_size = icon_->GetImage().size();
      badge_rect.Offset(
          (icon_size.width() - kAppBadgeIconSize) / 2,
          icon_size.height() - kBadgeBackgroundRadius - kAppBadgeIconSize / 2);
      badge_->SetBoundsRect(badge_rect);
    }

    rect.Inset(0, kGridIconDimension + kSearchTitleSpacing, 0, 0);
    rect.set_height(title_->GetPreferredSize().height());
    title_->SetBoundsRect(rect);

    if (rating_) {
      gfx::Rect rating_rect(rect);
      rating_rect.Inset(0, title_->GetPreferredSize().height(), 0, 0);
      rating_rect.set_size(rating_->GetPreferredSize());
      rating_rect.set_width(kSearchRatingSize);
      rating_->SetBoundsRect(rating_rect);
    }

    if (rating_star_) {
      gfx::Rect rating_star_rect(rect);
      rating_star_rect.Inset(
          kSearchRatingSize + kSearchRatingStarHorizontalSpacing,
          title_->GetPreferredSize().height() +
              kSearchRatingStarVerticalSpacing,
          0, 0);
      rating_star_rect.set_size(rating_star_->GetPreferredSize());
      rating_star_->SetBoundsRect(rating_star_rect);
    }

    if (price_) {
      gfx::Rect price_rect(rect);
      price_rect.Inset(rect.width() - kSearchPriceSize,
                       title_->GetPreferredSize().height(), 0, 0);
      price_rect.set_size(price_->GetPreferredSize());
      price_->SetBoundsRect(price_rect);
    }
  }
}

const char* SearchResultTileItemView::GetClassName() const {
  return "SearchResultTileItemView";
}

gfx::Size SearchResultTileItemView::CalculatePreferredSize() const {
  if (!item_)
    return gfx::Size();

  if (IsSuggestedAppTile())
    return gfx::Size(kGridTileWidth, kGridTileHeight);

  DCHECK(item_->display_type() == ash::SearchResultDisplayType::kTile);
  return gfx::Size(kSearchTileWidth, kSearchTileHeight);
}

bool SearchResultTileItemView::GetTooltipText(const gfx::Point& p,
                                              base::string16* tooltip) const {
  // Use the label to generate a tooltip, so that it will consider its text
  // truncation in making the tooltip. We do not want the label itself to have a
  // tooltip, so we only temporarily enable it to get the tooltip text from the
  // label, then disable it again.
  title_->SetHandlesTooltips(true);
  bool handled = title_->GetTooltipText(p, tooltip);
  title_->SetHandlesTooltips(false);
  return handled;
}

}  // namespace app_list
