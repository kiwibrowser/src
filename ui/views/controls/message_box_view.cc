// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/message_box_view.h"

#include <stddef.h>

#include "base/i18n/rtl.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/client_view.h"
#include "ui/views/window/dialog_delegate.h"

namespace {

const int kDefaultMessageWidth = 400;

// Paragraph separators are defined in
// http://www.unicode.org/Public/6.0.0/ucd/extracted/DerivedBidiClass.txt
//
// # Bidi_Class=Paragraph_Separator
//
// 000A          ; B # Cc       <control-000A>
// 000D          ; B # Cc       <control-000D>
// 001C..001E    ; B # Cc   [3] <control-001C>..<control-001E>
// 0085          ; B # Cc       <control-0085>
// 2029          ; B # Zp       PARAGRAPH SEPARATOR
bool IsParagraphSeparator(base::char16 c) {
  return ( c == 0x000A || c == 0x000D || c == 0x001C || c == 0x001D ||
           c == 0x001E || c == 0x0085 || c == 0x2029);
}

// Splits |text| into a vector of paragraphs.
// Given an example "\nabc\ndef\n\n\nhij\n", the split results should be:
// "", "abc", "def", "", "", "hij", and "".
void SplitStringIntoParagraphs(const base::string16& text,
                               std::vector<base::string16>* paragraphs) {
  paragraphs->clear();

  size_t start = 0;
  for (size_t i = 0; i < text.length(); ++i) {
    if (IsParagraphSeparator(text[i])) {
      paragraphs->push_back(text.substr(start, i - start));
      start = i + 1;
    }
  }
  paragraphs->push_back(text.substr(start, text.length() - start));
}

}  // namespace

namespace views {

///////////////////////////////////////////////////////////////////////////////
// MessageBoxView, public:

// static
const char MessageBoxView::kViewClassName[] = "MessageBoxView";

MessageBoxView::InitParams::InitParams(const base::string16& message)
    : options(NO_OPTIONS),
      message(message),
      message_width(kDefaultMessageWidth),
      inter_row_vertical_spacing(LayoutProvider::Get()->GetDistanceMetric(
          DISTANCE_RELATED_CONTROL_VERTICAL)) {}

MessageBoxView::InitParams::~InitParams() {
}

MessageBoxView::MessageBoxView(const InitParams& params)
    : prompt_field_(NULL),
      checkbox_(NULL),
      link_(NULL),
      message_width_(params.message_width) {
  Init(params);
}

MessageBoxView::~MessageBoxView() {}

base::string16 MessageBoxView::GetInputText() {
  return prompt_field_ ? prompt_field_->text() : base::string16();
}

bool MessageBoxView::IsCheckBoxSelected() {
  return checkbox_ ? checkbox_->checked() : false;
}

void MessageBoxView::SetCheckBoxLabel(const base::string16& label) {
  if (!checkbox_)
    checkbox_ = new Checkbox(label);
  else
    checkbox_->SetText(label);
  ResetLayoutManager();
}

void MessageBoxView::SetCheckBoxSelected(bool selected) {
  if (!checkbox_)
    return;
  checkbox_->SetChecked(selected);
}

void MessageBoxView::SetLink(const base::string16& text,
                             LinkListener* listener) {
  if (text.empty()) {
    DCHECK(!listener);
    delete link_;
    link_ = NULL;
  } else {
    DCHECK(listener);
    if (!link_) {
      link_ = new Link(text);
      link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    } else {
      link_->SetText(text);
    }
    link_->set_listener(listener);
  }
  ResetLayoutManager();
}

void MessageBoxView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kAlert;
}

///////////////////////////////////////////////////////////////////////////////
// MessageBoxView, View overrides:

void MessageBoxView::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.child == this && details.is_add) {
    if (prompt_field_)
      prompt_field_->SelectAll(true);

    NotifyAccessibilityEvent(ax::mojom::Event::kAlert, true);
  }
}

bool MessageBoxView::AcceleratorPressed(const ui::Accelerator& accelerator) {
  // We only accept Ctrl-C.
  DCHECK(accelerator.key_code() == 'C' && accelerator.IsCtrlDown());

  // We must not intercept Ctrl-C when we have a text box and it's focused.
  if (prompt_field_ && prompt_field_->HasFocus())
    return false;

  // Don't intercept Ctrl-C if we only use a single message label supporting
  // text selection.
  if (message_labels_.size() == 1u && message_labels_[0]->selectable())
    return false;

  ui::ScopedClipboardWriter scw(ui::CLIPBOARD_TYPE_COPY_PASTE);
  base::string16 text = message_labels_[0]->text();
  for (size_t i = 1; i < message_labels_.size(); ++i)
    text += message_labels_[i]->text();
  scw.WriteText(text);
  return true;
}

const char* MessageBoxView::GetClassName() const {
  return kViewClassName;
}

///////////////////////////////////////////////////////////////////////////////
// MessageBoxView, private:

void MessageBoxView::Init(const InitParams& params) {
  if (params.options & DETECT_DIRECTIONALITY) {
    std::vector<base::string16> texts;
    SplitStringIntoParagraphs(params.message, &texts);
    for (size_t i = 0; i < texts.size(); ++i) {
      Label* message_label =
          new Label(texts[i], style::CONTEXT_MESSAGE_BOX_BODY_TEXT);
      // Avoid empty multi-line labels, which have a height of 0.
      message_label->SetMultiLine(!texts[i].empty());
      message_label->SetAllowCharacterBreak(true);
      message_label->SetHorizontalAlignment(gfx::ALIGN_TO_HEAD);
      message_labels_.push_back(message_label);
    }
  } else {
    Label* message_label =
        new Label(params.message, style::CONTEXT_MESSAGE_BOX_BODY_TEXT);
    message_label->SetMultiLine(true);
    message_label->SetAllowCharacterBreak(true);
    message_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    message_labels_.push_back(message_label);
  }
  // Don't enable text selection if multiple labels are used, since text
  // selection can't span multiple labels.
  if (message_labels_.size() == 1u)
    message_labels_[0]->SetSelectable(true);

  if (params.options & HAS_PROMPT_FIELD) {
    prompt_field_ = new Textfield();
    prompt_field_->SetText(params.default_prompt);
    prompt_field_->SetAccessibleName(params.message);
  }

  inter_row_vertical_spacing_ = params.inter_row_vertical_spacing;

  ResetLayoutManager();
}

void MessageBoxView::ResetLayoutManager() {
  // Initialize the Grid Layout Manager used for this dialog box.
  GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));

  // Add the column set for the message displayed at the top of the dialog box.
  constexpr int kMessageViewColumnSetId = 0;
  ColumnSet* column_set = layout->AddColumnSet(kMessageViewColumnSetId);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, 1,
                        GridLayout::FIXED, message_width_, 0);

  const LayoutProvider* provider = LayoutProvider::Get();
  gfx::Insets horizontal_insets =
      provider->GetInsetsMetric(views::INSETS_DIALOG);
  horizontal_insets.Set(0, horizontal_insets.left(), 0,
                        horizontal_insets.right());

  // Column set for extra elements, if any.
  constexpr int kExtraViewColumnSetId = 1;
  if (prompt_field_ || checkbox_ || link_) {
    column_set = layout->AddColumnSet(kExtraViewColumnSetId);
    column_set->AddPaddingColumn(0, horizontal_insets.left());
    column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, 1,
                          GridLayout::USE_PREF, 0, 0);
    column_set->AddPaddingColumn(0, horizontal_insets.right());
  }

  views::View* message_contents = new views::View();
  // We explicitly set insets on the message contents instead of the scroll view
  // so that the scroll view borders are not capped by dialog insets.
  message_contents->SetBorder(CreateEmptyBorder(horizontal_insets));
  message_contents->SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  for (size_t i = 0; i < message_labels_.size(); ++i)
    message_contents->AddChildView(message_labels_[i]);
  ScrollView* scroll_view = new views::ScrollView();
  scroll_view->ClipHeightTo(0, provider->GetDistanceMetric(
                                   DISTANCE_DIALOG_SCROLLABLE_AREA_MAX_HEIGHT));

  scroll_view->SetContents(message_contents);
  layout->StartRow(0, kMessageViewColumnSetId);
  layout->AddView(scroll_view);

  views::DialogContentType trailing_content_type = views::TEXT;
  if (prompt_field_) {
    layout->AddPaddingRow(0, inter_row_vertical_spacing_);
    layout->StartRow(0, kExtraViewColumnSetId);
    layout->AddView(prompt_field_);
    trailing_content_type = views::CONTROL;
  }

  if (checkbox_) {
    layout->AddPaddingRow(0, inter_row_vertical_spacing_);
    layout->StartRow(0, kExtraViewColumnSetId);
    layout->AddView(checkbox_);
    trailing_content_type = views::TEXT;
  }

  if (link_) {
    layout->AddPaddingRow(0, inter_row_vertical_spacing_);
    layout->StartRow(0, kExtraViewColumnSetId);
    layout->AddView(link_);
    trailing_content_type = views::TEXT;
  }

  gfx::Insets border_insets =
      LayoutProvider::Get()->GetDialogInsetsForContentType(
          views::TEXT, trailing_content_type);
  // Horizontal insets have already been applied to the message contents and
  // controls as padding columns. Only apply the missing vertical insets.
  border_insets.Set(border_insets.top(), 0, border_insets.bottom(), 0);
  SetBorder(CreateEmptyBorder(border_insets));
}

}  // namespace views
