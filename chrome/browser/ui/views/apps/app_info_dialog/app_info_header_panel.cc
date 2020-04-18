// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/apps/app_info_dialog/app_info_header_panel.h"

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/chrome_app_icon.h"
#include "chrome/browser/extensions/chrome_app_icon_service.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/generated_resources.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/image_loader.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_icon_set.h"
#include "extensions/common/extension_resource.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "extensions/common/manifest_url_handlers.h"
#include "net/base/url_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"

namespace {

// Size of extension icon in top left of dialog.
const int kIconSize = 64;

}  // namespace

AppInfoHeaderPanel::AppInfoHeaderPanel(Profile* profile,
                                       const extensions::Extension* app)
    : AppInfoPanel(profile, app),
      weak_ptr_factory_(this) {
  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal,
      provider->GetInsetsMetric(views::INSETS_DIALOG_SUBSECTION),
      provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_HORIZONTAL)));

  CreateControls();
}

AppInfoHeaderPanel::~AppInfoHeaderPanel() {
}

void AppInfoHeaderPanel::CreateControls() {
  app_icon_view_ = new views::ImageView();
  app_icon_view_->SetImageSize(gfx::Size(kIconSize, kIconSize));
  AddChildView(app_icon_view_);

  app_icon_ = extensions::ChromeAppIconService::Get(profile_)->CreateIcon(
      this, app_->id(), extension_misc::EXTENSION_ICON_LARGE);

  // Create a vertical container to store the app's name and link.
  views::View* vertical_info_container = new views::View();
  auto vertical_container_layout =
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical);
  vertical_container_layout->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  vertical_info_container->SetLayoutManager(
      std::move(vertical_container_layout));
  AddChildView(vertical_info_container);

  views::Label* app_name_label = new views::Label(
      base::UTF8ToUTF16(app_->name()), views::style::CONTEXT_DIALOG_TITLE);
  app_name_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  vertical_info_container->AddChildView(app_name_label);

  if (CanShowAppInWebStore()) {
    view_in_store_link_ = new views::Link(
        l10n_util::GetStringUTF16(IDS_APPLICATION_INFO_WEB_STORE_LINK));
    view_in_store_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    view_in_store_link_->set_listener(this);
    vertical_info_container->AddChildView(view_in_store_link_);
  } else {
    // If there's no link, allow the app's name to take up multiple lines.
    // TODO(sashab): Limit the number of lines to 2.
    app_name_label->SetMultiLine(true);
  }
}

void AppInfoHeaderPanel::LinkClicked(views::Link* source, int event_flags) {
  DCHECK_EQ(source, view_in_store_link_);
  ShowAppInWebStore();
}

void AppInfoHeaderPanel::OnIconUpdated(extensions::ChromeAppIcon* icon) {
  app_icon_view_->SetImage(icon->image_skia());
}

void AppInfoHeaderPanel::ShowAppInWebStore() {
  DCHECK(CanShowAppInWebStore());
  Close();
  OpenLink(net::AppendQueryParameter(
      extensions::ManifestURL::GetDetailsURL(app_),
      extension_urls::kWebstoreSourceField,
      extension_urls::kLaunchSourceAppListInfoDialog));
}

bool AppInfoHeaderPanel::CanShowAppInWebStore() const {
  // Hide the webstore link for apps which were installed by default,
  // since this could leak user counts for OEM-specific apps.
  // Also hide Shared Modules because they are automatically installed
  // by Chrome when dependent Apps are installed.
  return app_->from_webstore() && !app_->was_installed_by_default() &&
      !app_->is_shared_module();
}
