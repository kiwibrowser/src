// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/app_list/model/folder_image.h"

#include <vector>

#include "ash/app_list/model/app_list_item.h"
#include "ash/app_list/model/app_list_item_list.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace app_list {

namespace {

constexpr int kItemIconDimension = 16;
constexpr float kFolderBubbleRadius = 23;
constexpr float kFolderBubbleOffsetY = 1;
// Keep this consistent with |kGridIconDimension| in app_list_constants.cc.
// TODO(hejq): Figure out a way to consolidate the two.
constexpr int kGridIconDimension = 48;

// Gets the size of a small app icon inside the folder icon.
gfx::Size ItemIconSize() {
  return gfx::Size(kItemIconDimension, kItemIconDimension);
}

// Generates the folder icon with the top 4 child item icons laid in 2x2 tile.
class FolderImageSource : public gfx::CanvasImageSource {
 public:
  typedef std::vector<gfx::ImageSkia> Icons;

  FolderImageSource(const Icons& icons, const gfx::Size& size);
  ~FolderImageSource() override;

 private:
  void DrawIcon(gfx::Canvas* canvas,
                const gfx::ImageSkia& icon,
                const gfx::Size icon_size,
                int x,
                int y);

  // gfx::CanvasImageSource overrides:
  void Draw(gfx::Canvas* canvas) override;

  Icons icons_;
  gfx::Size size_;

  DISALLOW_COPY_AND_ASSIGN(FolderImageSource);
};

FolderImageSource::FolderImageSource(const Icons& icons, const gfx::Size& size)
    : gfx::CanvasImageSource(size, false), icons_(icons), size_(size) {
  DCHECK(icons.size() <= FolderImage::kNumFolderTopItems);
}

FolderImageSource::~FolderImageSource() = default;

void FolderImageSource::DrawIcon(gfx::Canvas* canvas,
                                 const gfx::ImageSkia& icon,
                                 const gfx::Size icon_size,
                                 int x,
                                 int y) {
  if (icon.isNull())
    return;

  gfx::ImageSkia resized(gfx::ImageSkiaOperations::CreateResizedImage(
      icon, skia::ImageOperations::RESIZE_BEST, icon_size));
  canvas->DrawImageInt(resized, 0, 0, resized.width(), resized.height(), x, y,
                       resized.width(), resized.height(), true);
}

void FolderImageSource::Draw(gfx::Canvas* canvas) {
  cc::PaintFlags flags;
  // Draw circle for folder bubble.
  gfx::PointF bubble_center(size().width() / 2, size().height() / 2);
  bubble_center.Offset(0, -kFolderBubbleOffsetY);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  flags.setColor(FolderImage::kFolderBubbleColor);
  canvas->DrawCircle(bubble_center, kFolderBubbleRadius, flags);

  if (icons_.size() == 0)
    return;

  // Draw top items' icons.
  const gfx::Size item_icon_size(ItemIconSize());
  std::vector<gfx::Rect> top_icon_bounds =
      FolderImage::GetTopIconsBounds(gfx::Rect(size()));

  for (size_t i = 0; i < FolderImage::kNumFolderTopItems && i < icons_.size();
       ++i) {
    DrawIcon(canvas, icons_[i], item_icon_size, top_icon_bounds[i].x(),
             top_icon_bounds[i].y());
  }
}

}  // namespace

// static
const size_t FolderImage::kNumFolderTopItems = 4;
const SkColor FolderImage::kFolderBubbleColor =
    SkColorSetARGB(0x1F, 0xFF, 0xFF, 0xFF);

FolderImage::FolderImage(AppListItemList* item_list) : item_list_(item_list) {
  item_list_->AddObserver(this);
}

FolderImage::~FolderImage() {
  for (auto* item : top_items_)
    item->RemoveObserver(this);
  item_list_->RemoveObserver(this);
}

void FolderImage::UpdateIcon() {
  for (auto* item : top_items_)
    item->RemoveObserver(this);
  top_items_.clear();

  for (size_t i = 0; i < kNumFolderTopItems && i < item_list_->item_count();
       ++i) {
    AppListItem* item = item_list_->item_at(i);
    item->AddObserver(this);
    top_items_.push_back(item);
  }
  RedrawIconAndNotify();
}

// static
std::vector<gfx::Rect> FolderImage::GetTopIconsBounds(
    const gfx::Rect& folder_icon_bounds) {
  const int delta_to_center = 1;
  gfx::Point icon_center = folder_icon_bounds.CenterPoint();
  std::vector<gfx::Rect> top_icon_bounds;

  // Get the top left icon bounds.
  int left_x = icon_center.x() - kItemIconDimension - delta_to_center;
  int top_y = icon_center.y() - kItemIconDimension - delta_to_center;
  gfx::Rect top_left(left_x, top_y, kItemIconDimension, kItemIconDimension);
  top_icon_bounds.push_back(top_left);

  // Get the top right icon bounds.
  int right_x = icon_center.x() + delta_to_center;
  gfx::Rect top_right(right_x, top_y, kItemIconDimension, kItemIconDimension);
  top_icon_bounds.push_back(top_right);

  // Get the bottom left icon bounds.
  int bottom_y = icon_center.y() + delta_to_center;
  gfx::Rect bottom_left(left_x, bottom_y, kItemIconDimension,
                        kItemIconDimension);
  top_icon_bounds.push_back(bottom_left);

  // Get the bottom right icon bounds.
  gfx::Rect bottom_right(right_x, bottom_y, kItemIconDimension,
                         kItemIconDimension);
  top_icon_bounds.push_back(bottom_right);

  return top_icon_bounds;
}

gfx::Rect FolderImage::GetTargetIconRectInFolderForItem(
    AppListItem* item,
    const gfx::Rect& folder_icon_bounds) const {
  for (size_t i = 0; i < top_items_.size(); ++i) {
    if (item->id() == top_items_[i]->id()) {
      std::vector<gfx::Rect> rects = GetTopIconsBounds(folder_icon_bounds);
      return rects[i];
    }
  }

  gfx::Rect target_rect(folder_icon_bounds);
  target_rect.ClampToCenteredSize(ItemIconSize());
  return target_rect;
}

void FolderImage::AddObserver(FolderImageObserver* observer) {
  observers_.AddObserver(observer);
}

void FolderImage::RemoveObserver(FolderImageObserver* observer) {
  observers_.RemoveObserver(observer);
}

void FolderImage::ItemIconChanged() {
  // Note: Must update the image only (cannot simply call UpdateIcon), because
  // UpdateIcon removes and re-adds the FolderImage as an observer of the
  // AppListItems, which causes the current iterator to call ItemIconChanged
  // again, and goes into an infinite loop.
  RedrawIconAndNotify();
}

void FolderImage::OnListItemAdded(size_t index, AppListItem* item) {
  if (index < kNumFolderTopItems)
    UpdateIcon();
}

void FolderImage::OnListItemRemoved(size_t index, AppListItem* item) {
  if (index < kNumFolderTopItems)
    UpdateIcon();
}

void FolderImage::OnListItemMoved(size_t from_index,
                                  size_t to_index,
                                  AppListItem* item) {
  if (from_index < kNumFolderTopItems || to_index < kNumFolderTopItems)
    UpdateIcon();
}

void FolderImage::RedrawIconAndNotify() {
  FolderImageSource::Icons top_icons;
  for (const auto* item : top_items_)
    top_icons.push_back(item->icon());

  const gfx::Size icon_size = gfx::Size(kGridIconDimension, kGridIconDimension);
  icon_ = gfx::ImageSkia(
      std::make_unique<FolderImageSource>(top_icons, icon_size), icon_size);

  for (auto& observer : observers_)
    observer.OnFolderImageUpdated();
}

}  // namespace app_list
