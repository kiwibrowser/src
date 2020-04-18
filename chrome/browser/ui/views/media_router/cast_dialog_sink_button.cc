// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/cast_dialog_sink_button.h"

#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/throbber.h"
#include "ui/views/vector_icons.h"

#if defined(GOOGLE_CHROME_BUILD)
#include "chrome/browser/ui/media_router/internal/vector_icons/vector_icons.h"
#endif

namespace media_router {

namespace {

gfx::ImageSkia CreateSinkIcon(SinkIconType icon_type) {
  const gfx::VectorIcon* vector_icon;
  switch (icon_type) {
    case SinkIconType::CAST_AUDIO_GROUP:
      vector_icon = &kSpeakerGroupIcon;
      break;
    case SinkIconType::CAST_AUDIO:
      vector_icon = &kSpeakerIcon;
      break;
    case SinkIconType::EDUCATION:
      vector_icon = &kCastForEducationIcon;
      break;
    case SinkIconType::WIRED_DISPLAY:
      vector_icon = &kInputIcon;
      break;
// Use proprietary icons only in Chrome builds. The default TV icon is used
// instead for these sink types in Chromium builds.
#if defined(GOOGLE_CHROME_BUILD)
    case SinkIconType::MEETING:
      vector_icon = &vector_icons::kMeetIcon;
      break;
    case SinkIconType::HANGOUT:
      vector_icon = &vector_icons::kHangoutIcon;
      break;
#endif  // defined(GOOGLE_CHROME_BUILD)
    case SinkIconType::CAST:
    case SinkIconType::GENERIC:
    default:
      vector_icon = &kTvIcon;
      break;
  }
  constexpr int kPrimaryIconSize = 24;
  return gfx::CreateVectorIcon(*vector_icon, kPrimaryIconSize,
                               gfx::kChromeIconGrey);
}

std::unique_ptr<views::View> CreatePrimaryIconForSink(const UIMediaSink& sink) {
  auto icon_view = std::make_unique<views::ImageView>();
  icon_view->SetImage(CreateSinkIcon(sink.icon_type));
  return icon_view;
}

std::unique_ptr<views::View> CreateSecondaryIconForSink(
    const UIMediaSink& sink) {
  if (sink.state == UIMediaSinkState::CONNECTED) {
    constexpr int kSecondaryIconSize = 16;
    auto icon_view = std::make_unique<views::ImageView>();
    icon_view->SetImage(gfx::CreateVectorIcon(
        views::kMenuCheckIcon, kSecondaryIconSize, gfx::kChromeIconGrey));
    return icon_view;
  } else if (sink.state == UIMediaSinkState::CONNECTING) {
    auto throbber_view = std::make_unique<views::Throbber>();
    throbber_view->Start();
    return throbber_view;
  }
  return nullptr;
}

}  // namespace

CastDialogSinkButton::CastDialogSinkButton(
    views::ButtonListener* button_listener,
    const UIMediaSink& sink)
    : HoverButton(button_listener,
                  CreatePrimaryIconForSink(sink),
                  sink.friendly_name,
                  sink.status_text,
                  CreateSecondaryIconForSink(sink)),
      sink_(sink) {}

CastDialogSinkButton::~CastDialogSinkButton() {}

void CastDialogSinkButton::SetSelected(bool is_selected) {
  is_selected_ = is_selected;
  if (!is_selected_) {
    GetInkDrop()->SnapToHidden();
    GetInkDrop()->SetHovered(false);
  }
}

bool CastDialogSinkButton::OnMousePressed(const ui::MouseEvent& event) {
  // TODO(crbug.com/826089): Show a context menu on right click.
  if (event.IsRightMouseButton())
    return true;
  return HoverButton::OnMousePressed(event);
}

void CastDialogSinkButton::OnBlur() {
  Button::OnBlur();
  if (is_selected_)
    SnapInkDropToActivated();
}

base::string16 CastDialogSinkButton::GetActionText() const {
  return l10n_util::GetStringUTF16(sink_.state == UIMediaSinkState::CONNECTED
                                       ? IDS_MEDIA_ROUTER_STOP_CASTING_BUTTON
                                       : IDS_MEDIA_ROUTER_START_CASTING_BUTTON);
}

void CastDialogSinkButton::SnapInkDropToActivated() {
  GetInkDrop()->SnapToActivated();
}

}  // namespace media_router
