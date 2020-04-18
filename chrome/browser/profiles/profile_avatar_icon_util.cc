// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_avatar_icon_util.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkScalar.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/path.h"
#include "ui/gfx/skia_util.h"
#include "url/url_canon.h"

// Helper methods for transforming and drawing avatar icons.
namespace {

// Determine what the scaled height of the avatar icon should be for a
// specified width, to preserve the aspect ratio.
int GetScaledAvatarHeightForWidth(int width, const gfx::ImageSkia& avatar) {
  // Multiply the width by the inverted aspect ratio (height over
  // width), and then add 0.5 to ensure the int truncation rounds nicely.
  int scaled_height = width *
      ((float) avatar.height() / (float) avatar.width()) + 0.5f;
  return scaled_height;
}

// A CanvasImageSource that draws a sized and positioned avatar with an
// optional border independently of the scale factor.
class AvatarImageSource : public gfx::CanvasImageSource {
 public:
  enum AvatarPosition {
    POSITION_CENTER,
    POSITION_BOTTOM_CENTER,
  };

  enum AvatarBorder {
    BORDER_NONE,
    BORDER_NORMAL,
    BORDER_ETCHED,
  };

  AvatarImageSource(gfx::ImageSkia avatar,
                    const gfx::Size& canvas_size,
                    int width,
                    AvatarPosition position,
                    AvatarBorder border,
                    profiles::AvatarShape shape);

  AvatarImageSource(gfx::ImageSkia avatar,
                    const gfx::Size& canvas_size,
                    int width,
                    AvatarPosition position,
                    AvatarBorder border);

  ~AvatarImageSource() override;

  // CanvasImageSource override:
  void Draw(gfx::Canvas* canvas) override;

 private:
  gfx::ImageSkia avatar_;
  const gfx::Size canvas_size_;
  const int width_;
  const int height_;
  const AvatarPosition position_;
  const AvatarBorder border_;
  const profiles::AvatarShape shape_;

  DISALLOW_COPY_AND_ASSIGN(AvatarImageSource);
};

AvatarImageSource::AvatarImageSource(gfx::ImageSkia avatar,
                                     const gfx::Size& canvas_size,
                                     int width,
                                     AvatarPosition position,
                                     AvatarBorder border,
                                     profiles::AvatarShape shape)
    : gfx::CanvasImageSource(canvas_size, false),
      canvas_size_(canvas_size),
      width_(width),
      height_(GetScaledAvatarHeightForWidth(width, avatar)),
      position_(position),
      border_(border),
      shape_(shape) {
  avatar_ = gfx::ImageSkiaOperations::CreateResizedImage(
      avatar, skia::ImageOperations::RESIZE_BEST,
      gfx::Size(width_, height_));
}

AvatarImageSource::AvatarImageSource(gfx::ImageSkia avatar,
                                     const gfx::Size& canvas_size,
                                     int width,
                                     AvatarPosition position,
                                     AvatarBorder border)
    : AvatarImageSource(avatar,
                        canvas_size,
                        width,
                        position,
                        border,
                        profiles::SHAPE_SQUARE) {}

AvatarImageSource::~AvatarImageSource() {
}

void AvatarImageSource::Draw(gfx::Canvas* canvas) {
  // Center the avatar horizontally.
  int x = (canvas_size_.width() - width_) / 2;
  int y;

  if (position_ == POSITION_CENTER) {
    // Draw the avatar centered on the canvas.
    y = (canvas_size_.height() - height_) / 2;
  } else {
    // Draw the avatar on the bottom center of the canvas, leaving 1px below.
    y = canvas_size_.height() - height_ - 1;
  }

#if defined(OS_ANDROID)
  // Circular shape is only available on desktop platforms.
  DCHECK(shape_ != profiles::SHAPE_CIRCLE);
#else
  if (shape_ == profiles::SHAPE_CIRCLE) {
    // Draw the avatar on the bottom center of the canvas; overrides the
    // previous position specification to avoid leaving visible gap below the
    // avatar.
    y = canvas_size_.height() - height_;

    // Calculate the circular mask that will be used to display the avatar
    // image.
    gfx::Path circular_mask;
    circular_mask.addCircle(SkIntToScalar(canvas_size_.width() / 2),
                            SkIntToScalar(canvas_size_.height() / 2),
                            SkIntToScalar(canvas_size_.width() / 2));
    canvas->ClipPath(circular_mask, true);
  }
#endif

  canvas->DrawImageInt(avatar_, x, y);

  // The border should be square.
  int border_size = std::max(width_, height_);
  // Reset the x and y for the square border.
  x = (canvas_size_.width() - border_size) / 2;
  y = (canvas_size_.height() - border_size) / 2;

  if (border_ == BORDER_NORMAL) {
    // Draw a gray border on the inside of the avatar.
    SkColor border_color = SkColorSetARGB(83, 0, 0, 0);

    // Offset the rectangle by a half pixel so the border is drawn within the
    // appropriate pixels no matter the scale factor. Subtract 1 from the right
    // and bottom sizes to specify the endpoints, yielding -0.5.
    SkPath path;
    path.addRect(SkFloatToScalar(x + 0.5f),  // left
                 SkFloatToScalar(y + 0.5f),  // top
                 SkFloatToScalar(x + border_size - 0.5f),   // right
                 SkFloatToScalar(y + border_size - 0.5f));  // bottom

    cc::PaintFlags flags;
    flags.setColor(border_color);
    flags.setStyle(cc::PaintFlags::kStroke_Style);
    flags.setStrokeWidth(SkIntToScalar(1));

    canvas->DrawPath(path, flags);
  } else if (border_ == BORDER_ETCHED) {
    // Give the avatar an etched look by drawing a highlight on the bottom and
    // right edges.
    SkColor shadow_color = SkColorSetARGB(83, 0, 0, 0);
    SkColor highlight_color = SkColorSetARGB(96, 255, 255, 255);

    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kStroke_Style);
    flags.setStrokeWidth(SkIntToScalar(1));

    SkPath path;

    // Left and top shadows. To support higher scale factors than 1, position
    // the orthogonal dimension of each line on the half-pixel to separate the
    // pixel. For a vertical line, this means adding 0.5 to the x-value.
    path.moveTo(SkFloatToScalar(x + 0.5f), SkIntToScalar(y + height_));

    // Draw up to the top-left. Stop with the y-value at a half-pixel.
    path.rLineTo(SkIntToScalar(0), SkFloatToScalar(-height_ + 0.5f));

    // Draw right to the top-right, stopping within the last pixel.
    path.rLineTo(SkFloatToScalar(width_ - 0.5f), SkIntToScalar(0));

    flags.setColor(shadow_color);
    canvas->DrawPath(path, flags);

    path.reset();

    // Bottom and right highlights. Note that the shadows own the shared corner
    // pixels, so reduce the sizes accordingly.
    path.moveTo(SkIntToScalar(x + 1), SkFloatToScalar(y + height_ - 0.5f));

    // Draw right to the bottom-right.
    path.rLineTo(SkFloatToScalar(width_ - 1.5f), SkIntToScalar(0));

    // Draw up to the top-right.
    path.rLineTo(SkIntToScalar(0), SkFloatToScalar(-height_ + 1.5f));

    flags.setColor(highlight_color);
    canvas->DrawPath(path, flags);
  }
}

}  // namespace

namespace profiles {

struct IconResourceInfo {
  int resource_id;
  const char* filename;
  int label_id;
};

const int kAvatarIconWidth = 38;
const int kAvatarIconHeight = 31;
const SkColor kAvatarTutorialBackgroundColor = SkColorSetRGB(0x42, 0x85, 0xf4);
const SkColor kAvatarTutorialContentTextColor = SkColorSetRGB(0xc6, 0xda, 0xfc);
const SkColor kAvatarBubbleAccountsBackgroundColor =
    SkColorSetRGB(0xf3, 0xf3, 0xf3);
const SkColor kAvatarBubbleGaiaBackgroundColor =
    SkColorSetRGB(0xf5, 0xf5, 0xf5);
const SkColor kUserManagerBackgroundColor = SkColorSetRGB(0xee, 0xee, 0xee);

const char kDefaultUrlPrefix[] = "chrome://theme/IDR_PROFILE_AVATAR_";
const char kGAIAPictureFileName[] = "Google Profile Picture.png";
const char kHighResAvatarFolderName[] = "Avatars";

// The size of the function-static kDefaultAvatarIconResources array below.
const size_t kDefaultAvatarIconsCount = 27;

// The first 8 icons are generic.
const size_t kGenericAvatarIconsCount = 8;

// The avatar used as a placeholder (grey silhouette).
const size_t kPlaceholderAvatarIndex = 26;

gfx::Image GetSizedAvatarIcon(const gfx::Image& image,
                              bool is_rectangle,
                              int width,
                              int height,
                              AvatarShape shape) {
  if (!is_rectangle && image.Height() <= height)
    return image;

  gfx::Size size(width, height);

  // Source for a centered, sized icon. GAIA images get a border.
  std::unique_ptr<gfx::ImageSkiaSource> source(
      new AvatarImageSource(*image.ToImageSkia(), size, std::min(width, height),
                            AvatarImageSource::POSITION_CENTER,
                            AvatarImageSource::BORDER_NONE, shape));

  return gfx::Image(gfx::ImageSkia(std::move(source), size));
}

gfx::Image GetSizedAvatarIcon(const gfx::Image& image,
                              bool is_rectangle,
                              int width,
                              int height) {
  return GetSizedAvatarIcon(image, is_rectangle, width, height,
                            profiles::SHAPE_SQUARE);
}

gfx::Image GetAvatarIconForMenu(const gfx::Image& image,
                                bool is_rectangle) {
  return GetSizedAvatarIcon(
      image, is_rectangle, kAvatarIconWidth, kAvatarIconHeight);
}

gfx::Image GetAvatarIconForWebUI(const gfx::Image& image,
                                 bool is_rectangle) {
  return GetSizedAvatarIcon(image, is_rectangle,
                            kAvatarIconWidth, kAvatarIconHeight);
}

gfx::Image GetAvatarIconForTitleBar(const gfx::Image& image,
                                    bool is_gaia_image,
                                    int dst_width,
                                    int dst_height) {
  // The image requires no border or resizing.
  if (!is_gaia_image && image.Height() <= kAvatarIconHeight)
    return image;

  int size = std::min(std::min(kAvatarIconWidth, kAvatarIconHeight),
                      std::min(dst_width, dst_height));
  gfx::Size dst_size(dst_width, dst_height);

  // Source for a sized icon drawn at the bottom center of the canvas,
  // with an etched border (for GAIA images).
  std::unique_ptr<gfx::ImageSkiaSource> source(
      new AvatarImageSource(*image.ToImageSkia(), dst_size, size,
                            AvatarImageSource::POSITION_BOTTOM_CENTER,
                            is_gaia_image ? AvatarImageSource::BORDER_ETCHED
                                          : AvatarImageSource::BORDER_NONE));

  return gfx::Image(gfx::ImageSkia(std::move(source), dst_size));
}

SkBitmap GetAvatarIconAsSquare(const SkBitmap& source_bitmap,
                               int scale_factor) {
  SkBitmap square_bitmap;
  if ((source_bitmap.width() == scale_factor * profiles::kAvatarIconWidth) &&
      (source_bitmap.height() == scale_factor * profiles::kAvatarIconHeight)) {
    // Shave a couple of columns so the |source_bitmap| is more square. So when
    // resized to a square aspect ratio it looks pretty.
    gfx::Rect frame(scale_factor * profiles::kAvatarIconWidth,
                    scale_factor * profiles::kAvatarIconHeight);
    frame.Inset(scale_factor * 2, 0, scale_factor * 2, 0);
    source_bitmap.extractSubset(&square_bitmap, gfx::RectToSkIRect(frame));
  } else {
    // If not the avatar icon's aspect ratio, the image should be square.
    DCHECK(source_bitmap.width() == source_bitmap.height());
    square_bitmap = source_bitmap;
  }
  return square_bitmap;
}

// Helper methods for accessing, transforming and drawing avatar icons.
size_t GetDefaultAvatarIconCount() {
  return kDefaultAvatarIconsCount;
}

size_t GetGenericAvatarIconCount() {
  return kGenericAvatarIconsCount;
}

size_t GetPlaceholderAvatarIndex() {
  return kPlaceholderAvatarIndex;
}

int GetPlaceholderAvatarIconResourceID() {
  return IDR_PROFILE_AVATAR_PLACEHOLDER_LARGE;
}

std::string GetPlaceholderAvatarIconUrl() {
  return "chrome://theme/IDR_PROFILE_AVATAR_PLACEHOLDER_LARGE";
}

const IconResourceInfo* GetDefaultAvatarIconResourceInfo(size_t index) {
  CHECK_LT(index, kDefaultAvatarIconsCount);
  static const IconResourceInfo resource_info[kDefaultAvatarIconsCount] = {
      {IDR_PROFILE_AVATAR_0,
       "avatar_generic.png",
       IDS_DEFAULT_AVATAR_LABEL_0},
      {IDR_PROFILE_AVATAR_1,
       "avatar_generic_aqua.png",
       IDS_DEFAULT_AVATAR_LABEL_1},
      {IDR_PROFILE_AVATAR_2,
       "avatar_generic_blue.png",
       IDS_DEFAULT_AVATAR_LABEL_2},
      {IDR_PROFILE_AVATAR_3,
       "avatar_generic_green.png",
       IDS_DEFAULT_AVATAR_LABEL_3},
      {IDR_PROFILE_AVATAR_4,
       "avatar_generic_orange.png",
       IDS_DEFAULT_AVATAR_LABEL_4},
      {IDR_PROFILE_AVATAR_5,
       "avatar_generic_purple.png",
       IDS_DEFAULT_AVATAR_LABEL_5},
      {IDR_PROFILE_AVATAR_6,
       "avatar_generic_red.png",
       IDS_DEFAULT_AVATAR_LABEL_6},
      {IDR_PROFILE_AVATAR_7,
       "avatar_generic_yellow.png",
       IDS_DEFAULT_AVATAR_LABEL_7},
      {IDR_PROFILE_AVATAR_8,
       "avatar_secret_agent.png",
       IDS_DEFAULT_AVATAR_LABEL_8},
      {IDR_PROFILE_AVATAR_9,
       "avatar_superhero.png",
       IDS_DEFAULT_AVATAR_LABEL_9},
      {IDR_PROFILE_AVATAR_10,
       "avatar_volley_ball.png",
       IDS_DEFAULT_AVATAR_LABEL_10},
      {IDR_PROFILE_AVATAR_11,
       "avatar_businessman.png",
       IDS_DEFAULT_AVATAR_LABEL_11},
      {IDR_PROFILE_AVATAR_12,
       "avatar_ninja.png",
       IDS_DEFAULT_AVATAR_LABEL_12},
      {IDR_PROFILE_AVATAR_13,
       "avatar_alien.png",
       IDS_DEFAULT_AVATAR_LABEL_13},
      {IDR_PROFILE_AVATAR_14,
       "avatar_awesome.png",
       IDS_DEFAULT_AVATAR_LABEL_14},
      {IDR_PROFILE_AVATAR_15,
       "avatar_flower.png",
       IDS_DEFAULT_AVATAR_LABEL_15},
      {IDR_PROFILE_AVATAR_16,
       "avatar_pizza.png",
       IDS_DEFAULT_AVATAR_LABEL_16},
      {IDR_PROFILE_AVATAR_17,
       "avatar_soccer.png",
       IDS_DEFAULT_AVATAR_LABEL_17},
      {IDR_PROFILE_AVATAR_18,
       "avatar_burger.png",
       IDS_DEFAULT_AVATAR_LABEL_18},
      {IDR_PROFILE_AVATAR_19,
       "avatar_cat.png",
       IDS_DEFAULT_AVATAR_LABEL_19},
      {IDR_PROFILE_AVATAR_20,
       "avatar_cupcake.png",
       IDS_DEFAULT_AVATAR_LABEL_20},
      {IDR_PROFILE_AVATAR_21,
       "avatar_dog.png",
       IDS_DEFAULT_AVATAR_LABEL_21},
      {IDR_PROFILE_AVATAR_22,
       "avatar_horse.png",
       IDS_DEFAULT_AVATAR_LABEL_22},
      {IDR_PROFILE_AVATAR_23,
       "avatar_margarita.png",
       IDS_DEFAULT_AVATAR_LABEL_23},
      {IDR_PROFILE_AVATAR_24,
       "avatar_note.png",
       IDS_DEFAULT_AVATAR_LABEL_24},
      {IDR_PROFILE_AVATAR_25,
       "avatar_sun_cloud.png",
       IDS_DEFAULT_AVATAR_LABEL_25},
      {IDR_PROFILE_AVATAR_26, NULL, -1},
  };
  return &resource_info[index];
}

int GetDefaultAvatarIconResourceIDAtIndex(size_t index) {
  return GetDefaultAvatarIconResourceInfo(index)->resource_id;
}

const char* GetDefaultAvatarIconFileNameAtIndex(size_t index) {
  CHECK_NE(index, kPlaceholderAvatarIndex);
  return GetDefaultAvatarIconResourceInfo(index)->filename;
}

base::FilePath GetPathOfHighResAvatarAtIndex(size_t index) {
  const char* file_name = GetDefaultAvatarIconFileNameAtIndex(index);
  base::FilePath user_data_dir;
  CHECK(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir));
  return user_data_dir.AppendASCII(
      kHighResAvatarFolderName).AppendASCII(file_name);
}

std::string GetDefaultAvatarIconUrl(size_t index) {
  CHECK(IsDefaultAvatarIconIndex(index));
  return base::StringPrintf("%s%" PRIuS, kDefaultUrlPrefix, index);
}

int GetDefaultAvatarLabelResourceIDAtIndex(size_t index) {
  CHECK_NE(index, kPlaceholderAvatarIndex);
  return GetDefaultAvatarIconResourceInfo(index)->label_id;
}

bool IsDefaultAvatarIconIndex(size_t index) {
  return index < kDefaultAvatarIconsCount;
}

bool IsDefaultAvatarIconUrl(const std::string& url, size_t* icon_index) {
  DCHECK(icon_index);
  if (!base::StartsWith(url, kDefaultUrlPrefix, base::CompareCase::SENSITIVE))
    return false;

  int int_value = -1;
  if (base::StringToInt(base::StringPiece(url.begin() +
                                          strlen(kDefaultUrlPrefix),
                                          url.end()),
                        &int_value)) {
    if (int_value < 0 ||
        int_value >= static_cast<int>(kDefaultAvatarIconsCount))
      return false;
    *icon_index = int_value;
    return true;
  }

  return false;
}

std::unique_ptr<base::ListValue> GetDefaultProfileAvatarIconsAndLabels() {
  std::unique_ptr<base::ListValue> avatars(new base::ListValue());

  const size_t placeholder_avatar_index = profiles::GetPlaceholderAvatarIndex();
  for (size_t i = 0; i < profiles::GetDefaultAvatarIconCount() &&
                     i != placeholder_avatar_index;
       ++i) {
    std::unique_ptr<base::DictionaryValue> avatar_info(
        new base::DictionaryValue());
    avatar_info->SetString("url", profiles::GetDefaultAvatarIconUrl(i));
    avatar_info->SetString(
        "label", l10n_util::GetStringUTF16(
                     profiles::GetDefaultAvatarLabelResourceIDAtIndex(i)));

    avatars->Append(std::move(avatar_info));
  }
  return avatars;
}

}  // namespace profiles
