// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/collected_cookies_views.h"

#include <memory>

#include "base/macros.h"
#include "chrome/browser/browsing_data/browsing_data_appcache_helper.h"
#include "chrome/browser/browsing_data/browsing_data_channel_id_helper.h"
#include "chrome/browser/browsing_data/browsing_data_cookie_helper.h"
#include "chrome/browser/browsing_data/browsing_data_database_helper.h"
#include "chrome/browser/browsing_data/browsing_data_file_system_helper.h"
#include "chrome/browser/browsing_data/browsing_data_indexed_db_helper.h"
#include "chrome/browser/browsing_data/browsing_data_local_storage_helper.h"
#include "chrome/browser/browsing_data/cookies_tree_model.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/local_shared_objects_container.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/collected_cookies_infobar_delegate.h"
#include "chrome/browser/ui/views/cookie_info_view.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/web_contents.h"
#include "net/cookies/canonical_cookie.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/tabbed_pane/tabbed_pane.h"
#include "ui/views/controls/tree/tree_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

namespace {

// Dimensions of the tree views.
const int kTreeViewWidth = 400;
const int kTreeViewHeight = 125;

// Spacing constants used with non-Harmony dialogs.
const int kTabbedPaneTopPadding = 14;
const int kCookieInfoBottomPadding = 4;

// Adds a ColumnSet to |layout| to hold two buttons with padding between.
// Starts a new row with the added ColumnSet.
void StartNewButtonColumnSet(views::GridLayout* layout,
                             const int column_layout_id) {
  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  const int button_padding =
      provider->GetDistanceMetric(views::DISTANCE_RELATED_BUTTON_HORIZONTAL);
  const int button_size_limit =
      provider->GetDistanceMetric(views::DISTANCE_BUTTON_MAX_LINKABLE_WIDTH);

  views::ColumnSet* column_set = layout->AddColumnSet(column_layout_id);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER, 0,
                        views::GridLayout::USE_PREF, 0, 0);
  column_set->AddPaddingColumn(0, button_padding);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER, 0,
                        views::GridLayout::USE_PREF, 0, 0);
  column_set->LinkColumnSizes(0, 2, -1);
  column_set->set_linked_column_size_limit(button_size_limit);
  layout->StartRow(0, column_layout_id);
}

base::string16 GetAnnotationTextForSetting(ContentSetting setting) {
  switch (setting) {
    case CONTENT_SETTING_BLOCK:
      return l10n_util::GetStringUTF16(IDS_COLLECTED_COOKIES_BLOCKED_AUX_TEXT);
    case CONTENT_SETTING_ALLOW:
      return l10n_util::GetStringUTF16(IDS_COLLECTED_COOKIES_ALLOWED_AUX_TEXT);
    case CONTENT_SETTING_SESSION_ONLY:
      return l10n_util::GetStringUTF16(
          IDS_COLLECTED_COOKIES_CLEAR_ON_EXIT_AUX_TEXT);
    default:
      NOTREACHED() << "Unknown ContentSetting value: " << setting;
      return base::string16();
  }
}

}  // namespace

// This DrawingProvider allows TreeModelNodes to be annotated with auxiliary
// text. Annotated nodes will be drawn in a lighter color than normal to
// indicate that their state has changed, and will have their auxiliary text
// drawn on the trailing end of their row.
class CookiesTreeViewDrawingProvider : public views::TreeViewDrawingProvider {
 public:
  CookiesTreeViewDrawingProvider() {}
  ~CookiesTreeViewDrawingProvider() override {}

  void AnnotateNode(ui::TreeModelNode* node, const base::string16& text);

  SkColor GetTextColorForNode(views::TreeView* tree_view,
                              ui::TreeModelNode* node) override;
  base::string16 GetAuxiliaryTextForNode(views::TreeView* tree_view,
                                         ui::TreeModelNode* node) override;
  bool ShouldDrawIconForNode(views::TreeView* tree_view,
                             ui::TreeModelNode* node) override;

 private:
  std::map<ui::TreeModelNode*, base::string16> annotations_;
};

void CookiesTreeViewDrawingProvider::AnnotateNode(ui::TreeModelNode* node,
                                                  const base::string16& text) {
  annotations_[node] = text;
}

SkColor CookiesTreeViewDrawingProvider::GetTextColorForNode(
    views::TreeView* tree_view,
    ui::TreeModelNode* node) {
  SkColor color = TreeViewDrawingProvider::GetTextColorForNode(tree_view, node);
  if (annotations_.find(node) != annotations_.end())
    color = SkColorSetA(color, 0x80);
  return color;
}

base::string16 CookiesTreeViewDrawingProvider::GetAuxiliaryTextForNode(
    views::TreeView* tree_view,
    ui::TreeModelNode* node) {
  if (annotations_.find(node) != annotations_.end())
    return annotations_[node];
  return TreeViewDrawingProvider::GetAuxiliaryTextForNode(tree_view, node);
}

bool CookiesTreeViewDrawingProvider::ShouldDrawIconForNode(
    views::TreeView* tree_view,
    ui::TreeModelNode* node) {
  CookieTreeNode* cookie_node = static_cast<CookieTreeNode*>(node);
  return cookie_node->GetDetailedInfo().node_type !=
         CookieTreeNode::DetailedInfo::TYPE_HOST;
}

// A custom view that conditionally displays an infobar.
class InfobarView : public views::View {
 public:
  InfobarView() {
    content_ = new views::View;

    info_image_ = new views::ImageView();
    info_image_->SetImage(gfx::CreateVectorIcon(vector_icons::kInfoOutlineIcon,
                                                16, gfx::kChromeIconGrey));
    label_ = new views::Label();
  }
  ~InfobarView() override {}

  // Update the visibility of the infobar. If |is_visible| is true, a rule for
  // |setting| on |domain_name| was created.
  void UpdateVisibility(bool is_visible,
                        ContentSetting setting,
                        const base::string16& domain_name) {
    if (!is_visible) {
      SetVisible(false);
      return;
    }

    base::string16 label;
    switch (setting) {
      case CONTENT_SETTING_BLOCK:
        label = l10n_util::GetStringFUTF16(
            IDS_COLLECTED_COOKIES_BLOCK_RULE_CREATED, domain_name);
        break;

      case CONTENT_SETTING_ALLOW:
        label = l10n_util::GetStringFUTF16(
            IDS_COLLECTED_COOKIES_ALLOW_RULE_CREATED, domain_name);
        break;

      case CONTENT_SETTING_SESSION_ONLY:
        label = l10n_util::GetStringFUTF16(
            IDS_COLLECTED_COOKIES_SESSION_RULE_CREATED, domain_name);
        break;

      default:
        NOTREACHED();
    }
    label_->SetText(label);
    content_->Layout();
    SetVisible(true);
  }

 private:
  // Initialize contents and layout.
  void Init() {
    AddChildView(content_);
    // The containing dialog content view has no margins so that its
    // TabbedPane can span the full width of the dialog, but because of
    // that, InfobarView needs to impose its own horizontal margin.
    gfx::Insets dialog_insets =
        ChromeLayoutProvider::Get()->GetInsetsMetric(views::INSETS_DIALOG);
    // No top inset is needed because the control above the infobar imposes one,
    // but the button bar below the infobar has no margin, so a small bottom
    // inset is needed.
    gfx::Insets layout_insets(0, dialog_insets.left(),
                              ChromeLayoutProvider::Get()->GetDistanceMetric(
                                  DISTANCE_RELATED_CONTROL_VERTICAL_SMALL),
                              dialog_insets.right());
    content_->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::kHorizontal, layout_insets,
        ChromeLayoutProvider::Get()->GetDistanceMetric(
            DISTANCE_RELATED_CONTROL_HORIZONTAL_SMALL)));
    content_->AddChildView(info_image_);
    content_->AddChildView(label_);
    UpdateVisibility(false, CONTENT_SETTING_BLOCK, base::string16());
  }

  // views::View overrides.
  gfx::Size CalculatePreferredSize() const override {
    // Always return the preferred size, even if not currently visible. This
    // ensures that the layout manager always reserves space within the view
    // so it can be made visible when necessary. Otherwise, changing the
    // visibility of this view would require the entire dialog to be resized,
    // which is undesirable from both a UX and technical perspective.

    // Add space around the banner.
    gfx::Size size(content_->GetPreferredSize());
    size.Enlarge(0, 2 * ChromeLayoutProvider::Get()->GetDistanceMetric(
        views::DISTANCE_RELATED_CONTROL_VERTICAL));
    return size;
  }

  void Layout() override {
    ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
    const int vertical_spacing =
        provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL);
    content_->SetBounds(
        0, vertical_spacing, width(), height() - vertical_spacing);
  }

  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override {
    if (details.is_add && details.child == this)
      Init();
  }

  // Holds the info icon image and text label and renders the border.
  views::View* content_;
  // Info icon image.
  views::ImageView* info_image_;
  // The label responsible for rendering the text.
  views::Label* label_;

  DISALLOW_COPY_AND_ASSIGN(InfobarView);
};

///////////////////////////////////////////////////////////////////////////////
// CollectedCookiesViews, public:

CollectedCookiesViews::CollectedCookiesViews(content::WebContents* web_contents)
    : web_contents_(web_contents),
      allowed_label_(NULL),
      blocked_label_(NULL),
      allowed_cookies_tree_(NULL),
      blocked_cookies_tree_(NULL),
      block_allowed_button_(NULL),
      delete_allowed_button_(NULL),
      allow_blocked_button_(NULL),
      for_session_blocked_button_(NULL),
      cookie_info_view_(NULL),
      infobar_(NULL),
      status_changed_(false) {
  TabSpecificContentSettings* content_settings =
      TabSpecificContentSettings::FromWebContents(web_contents);
  registrar_.Add(this, chrome::NOTIFICATION_COLLECTED_COOKIES_SHOWN,
                 content::Source<TabSpecificContentSettings>(content_settings));
  constrained_window::ShowWebModalDialogViews(this, web_contents);
  chrome::RecordDialogCreation(chrome::DialogIdentifier::COLLECTED_COOKIES);
}

///////////////////////////////////////////////////////////////////////////////
// CollectedCookiesViews, views::DialogDelegate implementation:

base::string16 CollectedCookiesViews::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_COLLECTED_COOKIES_DIALOG_TITLE);
}

int CollectedCookiesViews::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK;
}

base::string16 CollectedCookiesViews::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(IDS_DONE);
}

bool CollectedCookiesViews::Accept() {
  // If the user closes our parent tab while we're still open, this method will
  // (eventually) be called in response to a WebContentsDestroyed() call from
  // the WebContentsImpl to its observers.  But since the InfoBarService is also
  // torn down in response to WebContentsDestroyed(), it may already be null.
  // Since the tab is going away anyway, we can just omit showing an infobar,
  // which prevents any attempt to access a null InfoBarService.
  if (status_changed_ && !web_contents_->IsBeingDestroyed()) {
    CollectedCookiesInfoBarDelegate::Create(
        InfoBarService::FromWebContents(web_contents_));
  }
  return true;
}

ui::ModalType CollectedCookiesViews::GetModalType() const {
  return ui::MODAL_TYPE_CHILD;
}

bool CollectedCookiesViews::ShouldShowCloseButton() const {
  return false;
}

views::View* CollectedCookiesViews::CreateExtraView() {
  // The code in |Init|, which runs before this does, needs the button pane to
  // already exist, so it is created there and this class holds ownership until
  // this method is called.
  return buttons_pane_.release();
}

///////////////////////////////////////////////////////////////////////////////
// CollectedCookiesViews, views::ButtonListener implementation:

void CollectedCookiesViews::ButtonPressed(views::Button* sender,
                                          const ui::Event& event) {
  if (sender == block_allowed_button_) {
    AddContentException(allowed_cookies_tree_, CONTENT_SETTING_BLOCK);
  } else if (sender == delete_allowed_button_) {
    allowed_cookies_tree_model_->DeleteCookieNode(
        static_cast<CookieTreeNode*>(allowed_cookies_tree_->GetSelectedNode()));
  } else if (sender == allow_blocked_button_) {
    AddContentException(blocked_cookies_tree_, CONTENT_SETTING_ALLOW);
  } else if (sender == for_session_blocked_button_) {
    AddContentException(blocked_cookies_tree_, CONTENT_SETTING_SESSION_ONLY);
  }
}

///////////////////////////////////////////////////////////////////////////////
// CollectedCookiesViews, views::TabbedPaneListener implementation:

void CollectedCookiesViews::TabSelectedAt(int index) {
  EnableControls();
  ShowCookieInfo();

  allowed_buttons_pane_->SetVisible(index == 0);
  blocked_buttons_pane_->SetVisible(index == 1);
}

///////////////////////////////////////////////////////////////////////////////
// CollectedCookiesViews, views::TreeViewController implementation:

void CollectedCookiesViews::OnTreeViewSelectionChanged(
    views::TreeView* tree_view) {
  EnableControls();
  ShowCookieInfo();
}

///////////////////////////////////////////////////////////////////////////////
// CollectedCookiesViews, views::View overrides:

gfx::Size CollectedCookiesViews::GetMinimumSize() const {
  // Allow UpdateWebContentsModalDialogPosition to clamp the dialog width.
  return gfx::Size(0, View::GetMinimumSize().height());
}

void CollectedCookiesViews::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  views::DialogDelegateView::ViewHierarchyChanged(details);
  if (details.is_add && details.child == this)
    Init();
}

////////////////////////////////////////////////////////////////////////////////
// CollectedCookiesViews, private:

CollectedCookiesViews::~CollectedCookiesViews() {
  allowed_cookies_tree_->SetModel(NULL);
  blocked_cookies_tree_->SetModel(NULL);
}

void CollectedCookiesViews::Init() {
  using views::GridLayout;

  GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));
  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  if (provider->UseExtraDialogPadding()) {
    SetBorder(
        views::CreateEmptyBorder(gfx::Insets(kTabbedPaneTopPadding, 0, 0, 0)));
  }

  const int single_column_layout_id = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(single_column_layout_id);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, 1,
                        GridLayout::USE_PREF, 0, 0);

  layout->StartRow(0, single_column_layout_id);
  views::TabbedPane* tabbed_pane = new views::TabbedPane();

  layout->AddView(tabbed_pane);
  // NOTE: Panes must be added after |tabbed_pane| has been added to its parent.
  base::string16 label_allowed = l10n_util::GetStringUTF16(
      IDS_COLLECTED_COOKIES_ALLOWED_COOKIES_TAB_LABEL);
  base::string16 label_blocked = l10n_util::GetStringUTF16(
      IDS_COLLECTED_COOKIES_BLOCKED_COOKIES_TAB_LABEL);
  tabbed_pane->AddTab(label_allowed, CreateAllowedPane());
  tabbed_pane->AddTab(label_blocked, CreateBlockedPane());
  tabbed_pane->SelectTabAt(0);
  tabbed_pane->set_listener(this);
  if (ChromeLayoutProvider::Get()->UseExtraDialogPadding()) {
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));
  }

  layout->StartRow(0, single_column_layout_id);
  cookie_info_view_ = new CookieInfoView();
  layout->AddView(cookie_info_view_);
  if (provider->UseExtraDialogPadding())
    layout->AddPaddingRow(0, kCookieInfoBottomPadding);

  layout->StartRow(0, single_column_layout_id);
  infobar_ = new InfobarView();
  layout->AddView(infobar_);

  buttons_pane_ = CreateButtonsPane();

  EnableControls();
  ShowCookieInfo();
}

views::View* CollectedCookiesViews::CreateAllowedPane() {
  TabSpecificContentSettings* content_settings =
      TabSpecificContentSettings::FromWebContents(web_contents_);

  // Create the controls that go into the pane.
  allowed_label_ = new views::Label(
      l10n_util::GetStringUTF16(IDS_COLLECTED_COOKIES_ALLOWED_COOKIES_LABEL),
      CONTEXT_BODY_TEXT_LARGE);
  allowed_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  allowed_cookies_tree_model_ =
      content_settings->allowed_local_shared_objects().CreateCookiesTreeModel();
  std::unique_ptr<CookiesTreeViewDrawingProvider> allowed_drawing_provider =
      std::make_unique<CookiesTreeViewDrawingProvider>();
  allowed_cookies_drawing_provider_ = allowed_drawing_provider.get();
  allowed_cookies_tree_ = new views::TreeView();
  allowed_cookies_tree_->SetModel(allowed_cookies_tree_model_.get());
  allowed_cookies_tree_->SetDrawingProvider(
      std::move(allowed_drawing_provider));
  allowed_cookies_tree_->SetRootShown(false);
  allowed_cookies_tree_->SetEditable(false);
  allowed_cookies_tree_->set_auto_expand_children(true);
  allowed_cookies_tree_->SetController(this);

  // Create the view that holds all the controls together.  This will be the
  // pane added to the tabbed pane.
  using views::GridLayout;

  views::View* pane = new views::View();
  GridLayout* layout =
      pane->SetLayoutManager(std::make_unique<views::GridLayout>(pane));

  pane->SetBorder(
      views::CreateEmptyBorder(ChromeLayoutProvider::Get()->GetInsetsMetric(
          views::INSETS_DIALOG_SUBSECTION)));
  int unrelated_vertical_distance =
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          views::DISTANCE_UNRELATED_CONTROL_VERTICAL);

  const int single_column_layout_id = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(single_column_layout_id);
  column_set->AddColumn(GridLayout::LEADING, GridLayout::FILL, 1,
                        GridLayout::USE_PREF, 0, 0);

  layout->StartRow(0, single_column_layout_id);
  layout->AddView(allowed_label_);
  layout->AddPaddingRow(0, unrelated_vertical_distance);

  layout->StartRow(1, single_column_layout_id);
  layout->AddView(CreateScrollView(allowed_cookies_tree_), 1, 1,
                  GridLayout::FILL, GridLayout::FILL, kTreeViewWidth,
                  kTreeViewHeight);
  layout->AddPaddingRow(0, unrelated_vertical_distance);

  return pane;
}

views::View* CollectedCookiesViews::CreateBlockedPane() {
  TabSpecificContentSettings* content_settings =
      TabSpecificContentSettings::FromWebContents(web_contents_);

  Profile* profile =
      Profile::FromBrowserContext(web_contents_->GetBrowserContext());
  PrefService* prefs = profile->GetPrefs();

  // Create the controls that go into the pane.
  blocked_label_ = new views::Label(
      l10n_util::GetStringUTF16(
          prefs->GetBoolean(prefs::kBlockThirdPartyCookies)
              ? IDS_COLLECTED_COOKIES_BLOCKED_THIRD_PARTY_BLOCKING_ENABLED
              : IDS_COLLECTED_COOKIES_BLOCKED_COOKIES_LABEL),
      CONTEXT_BODY_TEXT_LARGE);
  blocked_label_->SetMultiLine(true);
  blocked_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  blocked_label_->SizeToFit(kTreeViewWidth);
  blocked_cookies_tree_model_ =
      content_settings->blocked_local_shared_objects().CreateCookiesTreeModel();
  std::unique_ptr<CookiesTreeViewDrawingProvider> blocked_drawing_provider =
      std::make_unique<CookiesTreeViewDrawingProvider>();
  blocked_cookies_drawing_provider_ = blocked_drawing_provider.get();
  blocked_cookies_tree_ = new views::TreeView();
  blocked_cookies_tree_->SetModel(blocked_cookies_tree_model_.get());
  blocked_cookies_tree_->SetDrawingProvider(
      std::move(blocked_drawing_provider));
  blocked_cookies_tree_->SetRootShown(false);
  blocked_cookies_tree_->SetEditable(false);
  blocked_cookies_tree_->set_auto_expand_children(true);
  blocked_cookies_tree_->SetController(this);

  // Create the view that holds all the controls together.  This will be the
  // pane added to the tabbed pane.
  using views::GridLayout;

  views::View* pane = new views::View();
  GridLayout* layout =
      pane->SetLayoutManager(std::make_unique<views::GridLayout>(pane));
  pane->SetBorder(
      views::CreateEmptyBorder(ChromeLayoutProvider::Get()->GetInsetsMetric(
          views::INSETS_DIALOG_SUBSECTION)));
  int unrelated_vertical_distance =
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          views::DISTANCE_UNRELATED_CONTROL_VERTICAL);

  const int single_column_layout_id = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(single_column_layout_id);
  column_set->AddColumn(GridLayout::LEADING, GridLayout::FILL, 1,
                        GridLayout::USE_PREF, 0, 0);

  layout->StartRow(0, single_column_layout_id);
  layout->AddView(blocked_label_, 1, 1, GridLayout::FILL, GridLayout::FILL);
  layout->AddPaddingRow(0, unrelated_vertical_distance);

  layout->StartRow(1, single_column_layout_id);
  layout->AddView(
      CreateScrollView(blocked_cookies_tree_), 1, 1,
      GridLayout::FILL, GridLayout::FILL, kTreeViewWidth, kTreeViewHeight);
  layout->AddPaddingRow(0, unrelated_vertical_distance);

  return pane;
}

std::unique_ptr<views::View> CollectedCookiesViews::CreateButtonsPane() {
  auto view = std::make_unique<views::View>();
  view->SetLayoutManager(std::make_unique<views::FillLayout>());

  {
    auto allowed = std::make_unique<views::View>();
    views::GridLayout* layout = allowed->SetLayoutManager(
        std::make_unique<views::GridLayout>(allowed.get()));

    block_allowed_button_ = views::MdTextButton::CreateSecondaryUiButton(
        this, l10n_util::GetStringUTF16(IDS_COLLECTED_COOKIES_BLOCK_BUTTON));
    delete_allowed_button_ = views::MdTextButton::CreateSecondaryUiButton(
        this, l10n_util::GetStringUTF16(IDS_COOKIES_REMOVE_LABEL));
    StartNewButtonColumnSet(layout, 0);
    layout->AddView(block_allowed_button_);
    layout->AddView(delete_allowed_button_);

    allowed_buttons_pane_ = allowed.get();
    view->AddChildView(allowed.release());
  }

  {
    auto blocked = std::make_unique<views::View>();
    views::GridLayout* layout = blocked->SetLayoutManager(
        std::make_unique<views::GridLayout>(blocked.get()));
    blocked->SetVisible(false);

    allow_blocked_button_ = views::MdTextButton::CreateSecondaryUiButton(
        this, l10n_util::GetStringUTF16(IDS_COLLECTED_COOKIES_ALLOW_BUTTON));
    for_session_blocked_button_ = views::MdTextButton::CreateSecondaryUiButton(
        this,
        l10n_util::GetStringUTF16(IDS_COLLECTED_COOKIES_SESSION_ONLY_BUTTON));
    StartNewButtonColumnSet(layout, 0);
    layout->AddView(allow_blocked_button_);
    layout->AddView(for_session_blocked_button_);

    blocked_buttons_pane_ = blocked.get();
    view->AddChildView(blocked.release());
  }

  return view;
}

views::View* CollectedCookiesViews::CreateScrollView(views::TreeView* pane) {
  views::ScrollView* scroll_view =
      views::ScrollView::CreateScrollViewWithBorder();
  scroll_view->SetContents(pane);
  return scroll_view;
}

void CollectedCookiesViews::EnableControls() {
  bool enable_allowed_buttons = false;
  ui::TreeModelNode* node = allowed_cookies_tree_->GetSelectedNode();
  if (node) {
    CookieTreeNode* cookie_node = static_cast<CookieTreeNode*>(node);
    if (cookie_node->GetDetailedInfo().node_type ==
        CookieTreeNode::DetailedInfo::TYPE_HOST) {
      enable_allowed_buttons = static_cast<CookieTreeHostNode*>(
          cookie_node)->CanCreateContentException();
    }
  }
  block_allowed_button_->SetEnabled(enable_allowed_buttons);
  delete_allowed_button_->SetEnabled(node != NULL);

  bool enable_blocked_buttons = false;
  node = blocked_cookies_tree_->GetSelectedNode();
  if (node) {
    CookieTreeNode* cookie_node = static_cast<CookieTreeNode*>(node);
    if (cookie_node->GetDetailedInfo().node_type ==
        CookieTreeNode::DetailedInfo::TYPE_HOST) {
      enable_blocked_buttons = static_cast<CookieTreeHostNode*>(
          cookie_node)->CanCreateContentException();
    }
  }
  allow_blocked_button_->SetEnabled(enable_blocked_buttons);
  for_session_blocked_button_->SetEnabled(enable_blocked_buttons);
}

void CollectedCookiesViews::ShowCookieInfo() {
  ui::TreeModelNode* node = allowed_cookies_tree_->IsDrawn() ?
                            allowed_cookies_tree_->GetSelectedNode() : nullptr;

  if (!node && blocked_cookies_tree_->IsDrawn())
    node = blocked_cookies_tree_->GetSelectedNode();

  if (node) {
    CookieTreeNode* cookie_node = static_cast<CookieTreeNode*>(node);
    const CookieTreeNode::DetailedInfo detailed_info =
        cookie_node->GetDetailedInfo();

    if (detailed_info.node_type == CookieTreeNode::DetailedInfo::TYPE_COOKIE) {
      cookie_info_view_->SetCookie(detailed_info.cookie->Domain(),
                                   *detailed_info.cookie);
    } else {
      cookie_info_view_->ClearCookieDisplay();
    }
  } else {
    cookie_info_view_->ClearCookieDisplay();
  }
}

void CollectedCookiesViews::AddContentException(views::TreeView* tree_view,
                                                ContentSetting setting) {
  CookieTreeHostNode* host_node =
      static_cast<CookieTreeHostNode*>(tree_view->GetSelectedNode());
  Profile* profile =
      Profile::FromBrowserContext(web_contents_->GetBrowserContext());
  host_node->CreateContentException(
      CookieSettingsFactory::GetForProfile(profile).get(), setting);
  infobar_->UpdateVisibility(true, setting, host_node->GetTitle());
  status_changed_ = true;

  CookiesTreeViewDrawingProvider* provider =
      (tree_view == allowed_cookies_tree_) ? allowed_cookies_drawing_provider_
                                           : blocked_cookies_drawing_provider_;
  provider->AnnotateNode(tree_view->GetSelectedNode(),
                         GetAnnotationTextForSetting(setting));
  tree_view->SchedulePaint();
}

///////////////////////////////////////////////////////////////////////////////
// CollectedCookiesViews, content::NotificationObserver implementation:

void CollectedCookiesViews::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_COLLECTED_COOKIES_SHOWN, type);
  GetWidget()->Close();
}
