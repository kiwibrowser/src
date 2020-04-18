// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"

#include "base/bind.h"
#import "base/mac/mac_util.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/app_controller_mac.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/extensions/api/omnibox/omnibox_api.h"
#include "chrome/browser/extensions/extension_ui_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/translate/translate_service.h"
#include "chrome/browser/ui/autofill/save_card_bubble_controller_impl.h"
#include "chrome/browser/ui/browser_list.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/content_settings/content_setting_bubble_cocoa.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_cell.h"
#import "chrome/browser/ui/cocoa/location_bar/content_setting_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/keyword_hint_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/manage_passwords_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/page_info_bubble_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/save_credit_card_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/selected_keyword_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/star_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/translate_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/zoom_decoration.h"
#import "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"
#include "chrome/browser/ui/content_settings/content_setting_image_model.h"
#include "chrome/browser/ui/page_info/page_info_dialog.h"
#include "chrome/browser/ui/passwords/manage_passwords_ui_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chromium_strings.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#import "components/omnibox/browser/omnibox_popup_model.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "components/toolbar/vector_icons.h"
#include "components/translate/core/browser/language_state.h"
#include "components/variations/variations_associated_data.h"
#include "components/vector_icons/vector_icons.h"
#include "components/zoom/zoom_controller.h"
#include "components/zoom/zoom_event_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

using content::WebContents;

namespace {

const int kDefaultIconSize = 16;

// The minimum width the URL should have for the verbose state to be shown.
const int kMinURLWidth = 120;

// Color of the vector graphic icons when the location bar is dark.
// SkColorSetARGB(0xCC, 0xFF, 0xFF 0xFF);
const SkColor kMaterialDarkVectorIconColor = SK_ColorWHITE;

void NotReached(const gfx::Image& image) {
  // Mac Cocoa version should not receive asynchronously delivered favicons.
  NOTREACHED();
}

}  // namespace

// TODO(shess): This code is mostly copied from the gtk
// implementation.  Make sure it's all appropriate and flesh it out.

LocationBarViewMac::LocationBarViewMac(AutocompleteTextField* field,
                                       CommandUpdater* command_updater,
                                       Profile* profile,
                                       Browser* browser)
    : LocationBar(profile),
      ChromeOmniboxEditController(command_updater),
      omnibox_view_(new OmniboxViewMac(this, profile, command_updater, field)),
      field_(field),
      selected_keyword_decoration_(new SelectedKeywordDecoration()),
      page_info_decoration_(new PageInfoBubbleDecoration(this)),
      save_credit_card_decoration_(
          new SaveCreditCardDecoration(command_updater)),
      star_decoration_(new StarDecoration(command_updater)),
      translate_decoration_(new TranslateDecoration(command_updater)),
      zoom_decoration_(new ZoomDecoration(this)),
      keyword_hint_decoration_(new KeywordHintDecoration()),
      manage_passwords_decoration_(
          new ManagePasswordsDecoration(command_updater, this)),
      browser_(browser),
      location_bar_visible_(true),
      is_width_available_for_security_verbose_(false),
      security_level_(security_state::NONE) {
  std::vector<std::unique_ptr<ContentSettingImageModel>> models =
      ContentSettingImageModel::GenerateContentSettingImageModels();
  for (auto& model : models) {
    content_setting_decorations_.push_back(
        std::make_unique<ContentSettingDecoration>(std::move(model), this,
                                                   profile));
  }

  edit_bookmarks_enabled_.Init(
      bookmarks::prefs::kEditBookmarksEnabled, profile->GetPrefs(),
      base::Bind(&LocationBarViewMac::OnEditBookmarksEnabledChanged,
                 base::Unretained(this)));

  zoom::ZoomEventManager::GetForBrowserContext(profile)
      ->AddZoomEventManagerObserver(this);

  [[field_ cell] setIsPopupMode:
      !browser->SupportsWindowFeature(Browser::FEATURE_TABSTRIP)];

  // Sets images for the decorations, and performs a layout. This call ensures
  // that this class is in a consistent state after initialization.
  OnChanged();
}

LocationBarViewMac::~LocationBarViewMac() {
  // Disconnect from cell in case it outlives us.
  [[field_ cell] clearDecorations];

  zoom::ZoomEventManager::GetForBrowserContext(profile())
      ->RemoveZoomEventManagerObserver(this);
}

GURL LocationBarViewMac::GetDestinationURL() const {
  return destination_url();
}

WindowOpenDisposition LocationBarViewMac::GetWindowOpenDisposition() const {
  return disposition();
}

ui::PageTransition LocationBarViewMac::GetPageTransition() const {
  return transition();
}

void LocationBarViewMac::AcceptInput() {
  WindowOpenDisposition disposition =
      ui::WindowOpenDispositionFromNSEvent([NSApp currentEvent]);
  omnibox_view_->model()->AcceptInput(disposition, false);
}

void LocationBarViewMac::FocusLocation(bool select_all) {
  omnibox_view_->FocusLocation(select_all);
}

void LocationBarViewMac::FocusSearch() {
  omnibox_view_->EnterKeywordModeForDefaultSearchProvider();
}

void LocationBarViewMac::UpdateContentSettingsIcons() {
  if (RefreshContentSettingsDecorations())
    OnDecorationsChanged();
}

void LocationBarViewMac::UpdateManagePasswordsIconAndBubble() {
  WebContents* web_contents = GetWebContents();
  if (!web_contents)
    return;
  ManagePasswordsUIController::FromWebContents(web_contents)
      ->UpdateIconAndBubbleState(manage_passwords_decoration_->icon());
  OnDecorationsChanged();
}

void LocationBarViewMac::UpdateSaveCreditCardIcon() {
  WebContents* web_contents = GetWebContents();
  if (!web_contents)
    return;

  // |controller| may be nullptr due to lazy initialization.
  autofill::SaveCardBubbleControllerImpl* controller =
      autofill::SaveCardBubbleControllerImpl::FromWebContents(web_contents);
  bool enabled = controller && controller->IsIconVisible();
  if (!command_updater()->UpdateCommandEnabled(
          IDC_SAVE_CREDIT_CARD_FOR_PAGE, enabled)) {
    enabled = enabled && command_updater()->IsCommandEnabled(
        IDC_SAVE_CREDIT_CARD_FOR_PAGE);
  }
  save_credit_card_decoration_->SetIcon(IsLocationBarDark());
  save_credit_card_decoration_->SetVisible(enabled);
  OnDecorationsChanged();
}

void LocationBarViewMac::UpdateFindBarIconVisibility() {
  // TODO(crbug/651643): Implement for mac.
  NOTIMPLEMENTED();
}

void LocationBarViewMac::UpdateBookmarkStarVisibility() {
  star_decoration_->SetVisible(IsStarEnabled());
}

void LocationBarViewMac::UpdateZoomViewVisibility() {
  UpdateZoomDecoration(/*default_zoom_changed=*/false);
  OnChanged();
}

void LocationBarViewMac::UpdateLocationBarVisibility(bool visible,
                                                     bool animate) {
  // Track the target location bar visibility to avoid redundant transitions
  // being initiated when one is already in progress.
  if (visible != location_bar_visible_) {
    [[[BrowserWindowController browserWindowControllerForView:field_]
        toolbarController] updateVisibility:visible
                              withAnimation:animate];
    location_bar_visible_ = visible;
  }
}

void LocationBarViewMac::SaveStateToContents(WebContents* contents) {
  // TODO(shess): Why SaveStateToContents vs SaveStateToTab?
  omnibox_view_->SaveStateToTab(contents);
}

void LocationBarViewMac::Revert() {
  omnibox_view_->RevertAll();
}

const OmniboxView* LocationBarViewMac::GetOmniboxView() const {
  return omnibox_view_.get();
}

OmniboxView* LocationBarViewMac::GetOmniboxView() {
  return omnibox_view_.get();
}

LocationBarTesting* LocationBarViewMac::GetLocationBarForTesting() {
  return this;
}

bool LocationBarViewMac::GetBookmarkStarVisibility() {
  DCHECK(star_decoration_.get());
  return star_decoration_->IsVisible();
}

bool LocationBarViewMac::TestContentSettingImagePressed(size_t index) {
  if (index >= content_setting_decorations_.size())
    return false;

  // TODO(tapted): Use OnAccessibilityViewAction() here. Currently it's broken.
  ContentSettingDecoration* decoration =
      content_setting_decorations_[index].get();
  AutocompleteTextFieldCell* cell = [field_ cell];
  NSRect frame = [cell frameForDecoration:decoration inFrame:[field_ bounds]];
  decoration->OnMousePressed(frame, NSZeroPoint);
  return true;
}

bool LocationBarViewMac::IsContentSettingBubbleShowing(size_t index) {
  return index < content_setting_decorations_.size() &&
         content_setting_decorations_[index]->IsShowingBubble();
}

void LocationBarViewMac::SetEditable(bool editable) {
  [field_ setEditable:editable ? YES : NO];
  UpdateBookmarkStarVisibility();
  UpdateZoomDecoration(/*default_zoom_changed=*/false);
  Layout();
}

bool LocationBarViewMac::IsEditable() {
  return [field_ isEditable] ? true : false;
}

void LocationBarViewMac::SetStarred(bool starred) {
  if (star_decoration_->starred() == starred)
    return;

  star_decoration_->SetStarred(starred, IsLocationBarDark());
  UpdateBookmarkStarVisibility();
  OnDecorationsChanged();
}

void LocationBarViewMac::SetTranslateIconLit(bool on) {
  translate_decoration_->SetLit(on, IsLocationBarDark());
  OnDecorationsChanged();
}

void LocationBarViewMac::ZoomChangedForActiveTab(bool can_show_bubble) {
  bool changed = UpdateZoomDecoration(/*default_zoom_changed=*/false);
  if (changed)
    OnDecorationsChanged();

  if (can_show_bubble && zoom_decoration_->IsVisible())
    zoom_decoration_->ShowBubble(YES);
}

bool LocationBarViewMac::IsStarEnabled() const {
  return browser_defaults::bookmarks_enabled &&
         [field_ isEditable] &&
         !GetToolbarModel()->input_in_progress() &&
         edit_bookmarks_enabled_.GetValue() &&
         !IsBookmarkStarHiddenByExtension();
}

NSPoint LocationBarViewMac::GetBubblePointForDecoration(
    LocationBarDecoration* decoration) const {
  if (decoration == star_decoration_.get())
    DCHECK(IsStarEnabled());

  return [field_ bubblePointForDecoration:decoration];
}

NSPoint LocationBarViewMac::GetSaveCreditCardBubblePoint() const {
  return [field_ bubblePointForDecoration:save_credit_card_decoration_.get()];
}

NSPoint LocationBarViewMac::GetPageInfoBubblePoint() const {
  return [field_ bubblePointForDecoration:page_info_decoration_.get()];
}

void LocationBarViewMac::OnDecorationsChanged() {
  // TODO(shess): The field-editor frame and cursor rects should not
  // change, here.
  std::vector<LocationBarDecoration*> decorations = GetDecorations();
  for (auto* decoration : decorations)
    UpdateAccessibilityView(decoration);
  [field_ updateMouseTracking];
  [field_ resetFieldEditorFrameIfNeeded];
  [field_ setNeedsDisplay:YES];
}

// TODO(shess): This function should over time grow to closely match
// the views Layout() function.
void LocationBarViewMac::Layout() {
  AutocompleteTextFieldCell* cell = [field_ cell];

  // Reset the leading decorations.
  // TODO(shess): Shortly, this code will live somewhere else, like in
  // the constructor.  I am still wrestling with how best to deal with
  // right-hand decorations, which are not a static set.
  [cell clearDecorations];
  [cell addLeadingDecoration:selected_keyword_decoration_.get()];
  [cell addLeadingDecoration:page_info_decoration_.get()];
  [cell addTrailingDecoration:star_decoration_.get()];
  [cell addTrailingDecoration:translate_decoration_.get()];
  [cell addTrailingDecoration:zoom_decoration_.get()];
  [cell addTrailingDecoration:save_credit_card_decoration_.get()];
  [cell addTrailingDecoration:manage_passwords_decoration_.get()];

  for (const auto& decoration : content_setting_decorations_) {
    [cell addTrailingDecoration:decoration.get()];
  }

  [cell addTrailingDecoration:keyword_hint_decoration_.get()];

  // By default only the location icon is visible.
  selected_keyword_decoration_->SetVisible(false);
  keyword_hint_decoration_->SetVisible(false);
  page_info_decoration_->SetVisible(true);

  // Get the keyword to use for keyword-search and hinting.
  const base::string16 keyword = omnibox_view_->model()->keyword();
  base::string16 short_name;
  bool is_extension_keyword = false;
  if (!keyword.empty()) {
    short_name = TemplateURLServiceFactory::GetForProfile(profile())->
        GetKeywordShortName(keyword, &is_extension_keyword);
  }

  const bool is_keyword_hint = omnibox_view_->model()->is_keyword_hint();

  page_info_decoration_->SetFullLabel(nil);

  CGFloat available_width =
      [cell availableWidthInFrame:[[cell controlView] frame]];
  is_width_available_for_security_verbose_ = available_width >= kMinURLWidth;

  NSString* a11y_description = @"";
  if (!keyword.empty() && !is_keyword_hint) {
    // Switch from location icon to keyword mode.
    selected_keyword_decoration_->SetVisible(true);
    page_info_decoration_->SetVisible(false);
    selected_keyword_decoration_->SetKeyword(short_name, is_extension_keyword);
    // Note: the first time through this code path the
    // |selected_keyword_decoration_| has no image set because under Material
    // Design we need to set its color, which we cannot do until we know the
    // theme (by being installed in a browser window).
    selected_keyword_decoration_->SetImage(GetKeywordImage(keyword));
    a11y_description = selected_keyword_decoration_->GetAccessibilityLabel();
  } else if (!keyword.empty() && is_keyword_hint) {
    keyword_hint_decoration_->SetKeyword(short_name, is_extension_keyword);
    keyword_hint_decoration_->SetVisible(true);
    a11y_description = keyword_hint_decoration_->GetAccessibilityLabel();
  } else {
    UpdatePageInfoText();
  }
  [cell accessibilitySetOverrideValue:a11y_description
                         forAttribute:NSAccessibilityDescriptionAttribute];

  if (!page_info_decoration_->IsVisible())
    page_info_decoration_->ResetAnimation();

  // These need to change anytime the layout changes.
  // TODO(shess): Anytime the field editor might have changed, the
  // cursor rects almost certainly should have changed.  The tooltips
  // might change even when the rects don't change.
  OnDecorationsChanged();
}

void LocationBarViewMac::RedrawDecoration(LocationBarDecoration* decoration) {
  AutocompleteTextFieldCell* cell = [field_ cell];
  NSRect frame = [cell frameForDecoration:decoration
                                  inFrame:[field_ bounds]];
  if (!NSIsEmptyRect(frame))
    [field_ setNeedsDisplayInRect:frame];
}

void LocationBarViewMac::ResetTabState(WebContents* contents) {
  omnibox_view_->ResetTabState(contents);
}

void LocationBarViewMac::Update(const WebContents* contents) {
  UpdateManagePasswordsIconAndBubble();
  UpdateBookmarkStarVisibility();
  UpdateSaveCreditCardIcon();
  UpdateTranslateDecoration();
  UpdateZoomDecoration(/*default_zoom_changed=*/false);
  RefreshContentSettingsDecorations();
  if (contents) {
    omnibox_view_->OnTabChanged(contents);
    AnimatePageInfoIfPossible(contents);
  } else {
    omnibox_view_->Update();
  }

  OnChanged();
}

void LocationBarViewMac::UpdateWithoutTabRestore() {
  Update(nullptr);
}

void LocationBarViewMac::UpdateLocationIcon() {
  SkColor vector_icon_color = GetLocationBarIconColor();
  gfx::ImageSkia image_skia;
  if (GetPageInfoVerboseType() == PageInfoVerboseType::kEVCert) {
    image_skia = gfx::CreateVectorIcon(toolbar::kHttpsValidIcon,
                                       kDefaultIconSize, vector_icon_color);
  } else {
    image_skia = omnibox_view_->GetIcon(kDefaultIconSize, vector_icon_color,
                                        base::BindOnce(&NotReached));
  }

  NSImage* image = NSImageFromImageSkiaWithColorSpace(
      image_skia, base::mac::GetSRGBColorSpace());
  page_info_decoration_->SetImage(image);
  page_info_decoration_->SetLabelColor(vector_icon_color);

  Layout();
}

void LocationBarViewMac::UpdateColorsToMatchTheme() {
  // Update the location-bar icon.
  UpdateLocationIcon();

  // Make sure we're displaying the correct star color for Incognito mode. If
  // the window is in Incognito mode, switching between a theme and no theme
  // can move the window in and out of dark mode.
  star_decoration_->SetStarred(star_decoration_->starred(),
                               IsLocationBarDark());

  // Update the appearance of the text in the Omnibox.
  omnibox_view_->Update();
}

void LocationBarViewMac::OnAddedToWindow() {
  UpdateColorsToMatchTheme();
}

void LocationBarViewMac::OnThemeChanged() {
  UpdateColorsToMatchTheme();
}

void LocationBarViewMac::OnChanged() {
  AnimatePageInfoIfPossible(false);
  UpdateLocationIcon();
}

ToolbarModel* LocationBarViewMac::GetToolbarModel() {
  return browser_->toolbar_model();
}

const ToolbarModel* LocationBarViewMac::GetToolbarModel() const {
  return browser_->toolbar_model();
}

WebContents* LocationBarViewMac::GetWebContents() {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

void LocationBarViewMac::UpdatePageActionIcon(PageActionIconType) {
  // TODO(https://crbug.com/788051): Return page action icons for updating here
  // as update methods are migrated out of LocationBar to the
  // PageActionIconContainer interface.
  NOTIMPLEMENTED();
}

PageInfoVerboseType LocationBarViewMac::GetPageInfoVerboseType() const {
  if (omnibox_view_->IsEditingOrEmpty() ||
      omnibox_view_->model()->is_keyword_hint()) {
    return PageInfoVerboseType::kNone;
  } else if (GetToolbarModel()->GetSecurityLevel(false) ==
             security_state::EV_SECURE) {
    return PageInfoVerboseType::kEVCert;
  } else if (GetToolbarModel()->GetURL().SchemeIs(
                 extensions::kExtensionScheme)) {
    return PageInfoVerboseType::kExtension;
  } else if (GetToolbarModel()->GetURL().SchemeIs(content::kChromeUIScheme)) {
    return PageInfoVerboseType::kChrome;
  } else {
    return PageInfoVerboseType::kSecurity;
  }
}

bool LocationBarViewMac::HasSecurityVerboseText() const {
  if (GetPageInfoVerboseType() != PageInfoVerboseType::kSecurity)
    return false;

  return !GetToolbarModel()->GetSecureVerboseText().empty();
}

bool LocationBarViewMac::IsLocationBarDark() const {
  return [[field_ window] inIncognitoModeWithSystemTheme];
}

NSImage* LocationBarViewMac::GetKeywordImage(const base::string16& keyword) {
  const TemplateURL* template_url = TemplateURLServiceFactory::GetForProfile(
      profile())->GetTemplateURLForKeyword(keyword);
  if (template_url &&
      (template_url->type() == TemplateURL::OMNIBOX_API_EXTENSION)) {
    return extensions::OmniboxAPI::Get(profile())->
        GetOmniboxIcon(template_url->GetExtensionId()).AsNSImage();
  }

  SkColor icon_color =
      IsLocationBarDark() ? kMaterialDarkVectorIconColor : gfx::kGoogleBlue700;
  return NSImageFromImageSkiaWithColorSpace(
      gfx::CreateVectorIcon(vector_icons::kSearchIcon, kDefaultIconSize,
                            icon_color),
      base::mac::GetSRGBColorSpace());
}

SkColor LocationBarViewMac::GetLocationBarIconColor() const {
  bool in_dark_mode = IsLocationBarDark();
  if (in_dark_mode)
    return kMaterialDarkVectorIconColor;

  if (GetPageInfoVerboseType() == PageInfoVerboseType::kEVCert)
    return gfx::kGoogleGreen700;

  security_state::SecurityLevel security_level =
      GetToolbarModel()->GetSecurityLevel(false);

  if (security_level == security_state::NONE ||
      security_level == security_state::HTTP_SHOW_WARNING) {
    return gfx::kChromeIconGrey;
  }

  NSColor* srgb_color =
      OmniboxViewMac::GetSecureTextColor(security_level, in_dark_mode);
  NSColor* device_color =
      [srgb_color colorUsingColorSpace:[NSColorSpace deviceRGBColorSpace]];
  return skia::NSDeviceColorToSkColor(device_color);
}

void LocationBarViewMac::PostNotification(NSString* notification) {
  [[NSNotificationCenter defaultCenter] postNotificationName:notification
                                        object:[NSValue valueWithPointer:this]];
}

void LocationBarViewMac::OnEditBookmarksEnabledChanged() {
  UpdateBookmarkStarVisibility();
  OnChanged();
}

void LocationBarViewMac::UpdatePageInfoText() {
  // Don't change the label if the bubble is in the process of animating
  // out the old one.
  if (page_info_decoration_->AnimatingOut())
    return;

  base::string16 label;
  PageInfoVerboseType type = GetPageInfoVerboseType();
  if (type == PageInfoVerboseType::kEVCert) {
    label = GetToolbarModel()->GetSecureVerboseText();
  } else if (type == PageInfoVerboseType::kExtension && GetWebContents()) {
    label = extensions::ui_util::GetEnabledExtensionNameForUrl(
        GetToolbarModel()->GetURL(), GetWebContents()->GetBrowserContext());
  } else if (type == PageInfoVerboseType::kChrome) {
    label = l10n_util::GetStringUTF16(IDS_SHORT_PRODUCT_NAME);
  } else if (type == PageInfoVerboseType::kSecurity &&
             HasSecurityVerboseText()) {
    if (is_width_available_for_security_verbose_)
      label = GetToolbarModel()->GetSecureVerboseText();
  }

  page_info_decoration_->SetFullLabel(base::SysUTF16ToNSString(label));
}

bool LocationBarViewMac::RefreshContentSettingsDecorations() {
  const bool input_in_progress = GetToolbarModel()->input_in_progress();
  WebContents* web_contents = input_in_progress ?
      NULL : browser_->tab_strip_model()->GetActiveWebContents();
  bool icons_updated = false;
  for (const auto& decoration : content_setting_decorations_)
    icons_updated |= decoration->UpdateFromWebContents(web_contents);
  return icons_updated;
}

void LocationBarViewMac::UpdateTranslateDecoration() {
  if (!TranslateService::IsTranslateBubbleEnabled())
    return;

  WebContents* web_contents = GetWebContents();
  if (!web_contents)
    return;
  translate::LanguageState& language_state =
      ChromeTranslateClient::FromWebContents(web_contents)->GetLanguageState();
  bool enabled = language_state.translate_enabled();
  if (!command_updater()->UpdateCommandEnabled(IDC_TRANSLATE_PAGE, enabled)) {
    enabled = enabled && command_updater()->IsCommandEnabled(
        IDC_TRANSLATE_PAGE);
  }
  translate_decoration_->SetVisible(enabled);
  translate_decoration_->SetLit(language_state.IsPageTranslated(),
                                IsLocationBarDark());
}

bool LocationBarViewMac::UpdateZoomDecoration(bool default_zoom_changed) {
  WebContents* web_contents = GetWebContents();
  if (!web_contents)
    return false;

  return zoom_decoration_->UpdateIfNecessary(
      zoom::ZoomController::FromWebContents(web_contents), default_zoom_changed,
      IsLocationBarDark());
}

void LocationBarViewMac::AnimatePageInfoIfPossible(bool tab_changed) {
  using SecurityLevel = security_state::SecurityLevel;
  SecurityLevel new_security_level = GetToolbarModel()->GetSecurityLevel(false);
  bool is_new_security_level = security_level_ != new_security_level;
  SecurityLevel old_security_level = security_level_;
  security_level_ = new_security_level;

  if (tab_changed)
    page_info_decoration_->ResetAnimation();

  // Animation is only applicable for the security verbose and if the icon
  // isn't updated from a tab switch.
  if (GetPageInfoVerboseType() != PageInfoVerboseType::kSecurity ||
      !HasSecurityVerboseText() || tab_changed) {
    page_info_decoration_->ShowWithoutAnimation();
    return;
  }

  // Do not animate HTTP_SHOW_WARNING to DANGEROUS transitions because they look
  // messy/confusing.
  if (old_security_level == security_state::HTTP_SHOW_WARNING &&
      security_level_ == security_state::DANGEROUS) {
    page_info_decoration_->ShowWithoutAnimation();
    return;
  }

  if (is_width_available_for_security_verbose_) {
    if (!is_new_security_level && page_info_decoration_->HasAnimatedOut())
      page_info_decoration_->AnimateIn(false);
    else if (!CanAnimateSecurityLevel(new_security_level))
      page_info_decoration_->ShowWithoutAnimation();
    else if (is_new_security_level)
      page_info_decoration_->AnimateIn();
  } else {
    page_info_decoration_->AnimateOut();
  }
}

bool LocationBarViewMac::CanAnimateSecurityLevel(
    security_state::SecurityLevel level) const {
  return level == security_state::DANGEROUS ||
         level == security_state::HTTP_SHOW_WARNING;
}

void LocationBarViewMac::UpdateAccessibilityView(
    LocationBarDecoration* decoration) {
  if (!decoration->IsVisible())
    return;
  // This uses |frame| instead of |bounds| because the accessibility views are
  // parented to the toolbar.
  NSRect apparent_frame =
      [[field_ cell] frameForDecoration:decoration inFrame:[field_ frame]];

  // This is a bit subtle:
  // The decorations' accessibility views can become key to allow keyboard
  // access to the location bar decorations, but Cocoa's automatic key view loop
  // sorts by top-left coordinate. Since the omnibox's top-left coordinate is
  // before its leading decorations, the omnibox would sort before its own
  // leading decorations, which was logical but visually unintuitive. Therefore,
  // for leading decorations, this method moves their frame to be "just before"
  // the omnibox in automatic key view loop order, and gives them an apparent
  // frame (see DecorationAccessibilityView) so that they still paint their
  // focus rings at the right place.
  //
  // TODO(lgrey): This hack doesn't work in RTL layouts, but the layout of the
  // omnibox is currently screwed up in RTL layouts anyway. See
  // https://crbug.com/715627.
  NSRect real_frame = apparent_frame;
  int left_index = [[field_ cell] leadingDecorationIndex:decoration];

  // If there are ever too many leading views, the fake x-coords might land
  // before the button preceding the omnibox in the key view order. This
  // threshold is just a guess.
  DCHECK_LT(left_index, 10);
  if (left_index != -1) {
    CGFloat delta = left_index + 1;
    real_frame.origin.x =
        cocoa_l10n_util::ShouldDoExperimentalRTLLayout()
            ? NSMaxX([field_ frame]) + delta - NSWidth(real_frame)
            : NSMinX([field_ frame]) - delta;
  }
  decoration->UpdateAccessibilityView(apparent_frame);
  [decoration->GetAccessibilityView() setFrame:real_frame];
  [decoration->GetAccessibilityView() setNeedsDisplayInRect:apparent_frame];
}

std::vector<LocationBarDecoration*> LocationBarViewMac::GetDecorations() {
  std::vector<LocationBarDecoration*> decorations;

  // TODO(ellyjones): page actions and keyword hints are not included right
  // now. Keyword hints have no useful tooltip (issue 752592), and page actions
  // are likewise.
  decorations.push_back(selected_keyword_decoration_.get());
  decorations.push_back(page_info_decoration_.get());
  decorations.push_back(save_credit_card_decoration_.get());
  decorations.push_back(star_decoration_.get());
  decorations.push_back(translate_decoration_.get());
  decorations.push_back(zoom_decoration_.get());
  decorations.push_back(manage_passwords_decoration_.get());
  for (const auto& decoration : content_setting_decorations_)
    decorations.push_back(decoration.get());
  return decorations;
}

void LocationBarViewMac::OnDefaultZoomLevelChanged() {
  if (UpdateZoomDecoration(/*default_zoom_changed=*/true))
    OnDecorationsChanged();
}

std::vector<NSView*> LocationBarViewMac::GetDecorationAccessibilityViews() {
  std::vector<LocationBarDecoration*> decorations = GetDecorations();
  std::vector<NSView*> views;
  for (auto* decoration : decorations)
    views.push_back(decoration->GetAccessibilityView());
  return views;
}
