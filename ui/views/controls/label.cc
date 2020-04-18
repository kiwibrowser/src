// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/label.h"

#include <stddef.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/default_style.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/text_elider.h"
#include "ui/gfx/text_utils.h"
#include "ui/native_theme/native_theme.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/background.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/native_cursor.h"
#include "ui/views/selection_controller.h"

namespace views {
namespace {
// Returns additional Insets applied to |label->GetContentsBounds()| to obtain
// the text bounds. GetContentsBounds() includes the Border, but not any
// additional insets used by the Label (e.g. for a focus ring).
gfx::Insets NonBorderInsets(const Label& label) {
  return label.GetInsets() - label.View::GetInsets();
}
}  // namespace

const char Label::kViewClassName[] = "Label";

Label::Label() : Label(base::string16()) {
}

Label::Label(const base::string16& text)
    : Label(text, style::CONTEXT_LABEL, style::STYLE_PRIMARY) {}

Label::Label(const base::string16& text, int text_context, int text_style)
    : text_context_(text_context), context_menu_contents_(this) {
  Init(text, style::GetFont(text_context, text_style));
  SetLineHeight(style::GetLineHeight(text_context, text_style));

  // If an explicit style is given, ignore color changes due to the NativeTheme.
  if (text_style != style::STYLE_PRIMARY)
    SetEnabledColor(style::GetColor(*this, text_context, text_style));
}

Label::Label(const base::string16& text, const CustomFont& font)
    : text_context_(style::CONTEXT_LABEL), context_menu_contents_(this) {
  Init(text, font.font_list);
}

Label::~Label() {
}

// static
const gfx::FontList& Label::GetDefaultFontList() {
  return style::GetFont(style::CONTEXT_LABEL, style::STYLE_PRIMARY);
}

void Label::SetFontList(const gfx::FontList& font_list) {
  full_text_->SetFontList(font_list);
  ResetLayout();
}

void Label::SetText(const base::string16& new_text) {
  if (new_text == text())
    return;
  full_text_->SetText(new_text);
  ResetLayout();
  stored_selection_range_ = gfx::Range::InvalidRange();
}

void Label::SetAutoColorReadabilityEnabled(bool enabled) {
  if (auto_color_readability_ == enabled)
    return;
  auto_color_readability_ = enabled;
  RecalculateColors();
}

void Label::SetEnabledColor(SkColor color) {
  if (enabled_color_set_ && requested_enabled_color_ == color)
    return;
  requested_enabled_color_ = color;
  enabled_color_set_ = true;
  RecalculateColors();
}

void Label::SetBackgroundColor(SkColor color) {
  if (background_color_set_ && background_color_ == color)
    return;
  background_color_ = color;
  background_color_set_ = true;
  RecalculateColors();
}

void Label::SetSelectionTextColor(SkColor color) {
  if (selection_text_color_set_ && requested_selection_text_color_ == color)
    return;
  requested_selection_text_color_ = color;
  selection_text_color_set_ = true;
  RecalculateColors();
}

void Label::SetSelectionBackgroundColor(SkColor color) {
  if (selection_background_color_set_ && selection_background_color_ == color)
    return;
  selection_background_color_ = color;
  selection_background_color_set_ = true;
  RecalculateColors();
}

void Label::SetShadows(const gfx::ShadowValues& shadows) {
  if (full_text_->shadows() == shadows)
    return;
  full_text_->set_shadows(shadows);
  ResetLayout();
}

void Label::SetSubpixelRenderingEnabled(bool subpixel_rendering_enabled) {
  if (subpixel_rendering_enabled_ == subpixel_rendering_enabled)
    return;
  subpixel_rendering_enabled_ = subpixel_rendering_enabled;
  RecalculateColors();
}

void Label::SetHorizontalAlignment(gfx::HorizontalAlignment alignment) {
  alignment = gfx::MaybeFlipForRTL(alignment);
  if (horizontal_alignment() == alignment)
    return;
  full_text_->SetHorizontalAlignment(alignment);
  ResetLayout();
}

void Label::SetLineHeight(int height) {
  if (line_height() == height)
    return;
  full_text_->SetMinLineHeight(height);
  ResetLayout();
}

void Label::SetMultiLine(bool multi_line) {
  DCHECK(!multi_line || (elide_behavior_ == gfx::ELIDE_TAIL ||
                         elide_behavior_ == gfx::NO_ELIDE));
  if (this->multi_line() == multi_line)
    return;
  multi_line_ = multi_line;
  full_text_->SetMultiline(multi_line);
  full_text_->SetReplaceNewlineCharsWithSymbols(!multi_line);
  ResetLayout();
}

void Label::SetMaxLines(int max_lines) {
  if (max_lines_ == max_lines)
    return;
  max_lines_ = max_lines;
  ResetLayout();
}

void Label::SetObscured(bool obscured) {
  if (this->obscured() == obscured)
    return;
  full_text_->SetObscured(obscured);
  if (obscured)
    SetSelectable(false);
  ResetLayout();
}

void Label::SetAllowCharacterBreak(bool allow_character_break) {
  const gfx::WordWrapBehavior behavior =
      allow_character_break ? gfx::WRAP_LONG_WORDS : gfx::TRUNCATE_LONG_WORDS;
  if (full_text_->word_wrap_behavior() == behavior)
    return;
  full_text_->SetWordWrapBehavior(behavior);
  if (multi_line()) {
    ResetLayout();
  }
}

void Label::SetElideBehavior(gfx::ElideBehavior elide_behavior) {
  DCHECK(!multi_line() || (elide_behavior_ == gfx::ELIDE_TAIL ||
                           elide_behavior_ == gfx::NO_ELIDE));
  if (elide_behavior_ == elide_behavior)
    return;
  elide_behavior_ = elide_behavior;
  ResetLayout();
}

void Label::SetTooltipText(const base::string16& tooltip_text) {
  DCHECK(handles_tooltips_);
  tooltip_text_ = tooltip_text;
}

void Label::SetHandlesTooltips(bool enabled) {
  handles_tooltips_ = enabled;
}

void Label::SizeToFit(int fixed_width) {
  DCHECK(multi_line());
  DCHECK_EQ(0, max_width_);
  fixed_width_ = fixed_width;
  SizeToPreferredSize();
}

void Label::SetMaximumWidth(int max_width) {
  DCHECK(multi_line());
  DCHECK_EQ(0, fixed_width_);
  max_width_ = max_width;
  SizeToPreferredSize();
}

base::string16 Label::GetDisplayTextForTesting() {
  ClearDisplayText();
  MaybeBuildDisplayText();
  return display_text_ ? display_text_->GetDisplayText() : base::string16();
}

bool Label::IsSelectionSupported() const {
  return !obscured() && full_text_->IsSelectionSupported();
}

bool Label::SetSelectable(bool value) {
  if (value == selectable())
    return true;

  if (!value) {
    ClearSelection();
    stored_selection_range_ = gfx::Range::InvalidRange();
    selection_controller_.reset();
    return true;
  }

  DCHECK(!stored_selection_range_.IsValid());
  if (!IsSelectionSupported())
    return false;

  selection_controller_ = std::make_unique<SelectionController>(this);
  return true;
}

bool Label::HasSelection() const {
  const gfx::RenderText* render_text = GetRenderTextForSelectionController();
  return render_text ? !render_text->selection().is_empty() : false;
}

void Label::SelectAll() {
  gfx::RenderText* render_text = GetRenderTextForSelectionController();
  if (!render_text)
    return;
  render_text->SelectAll(false);
  SchedulePaint();
}

void Label::ClearSelection() {
  gfx::RenderText* render_text = GetRenderTextForSelectionController();
  if (!render_text)
    return;
  render_text->ClearSelection();
  SchedulePaint();
}

void Label::SelectRange(const gfx::Range& range) {
  gfx::RenderText* render_text = GetRenderTextForSelectionController();
  if (render_text && render_text->SelectRange(range))
    SchedulePaint();
}

int Label::GetBaseline() const {
  return GetInsets().top() + font_list().GetBaseline();
}

gfx::Size Label::CalculatePreferredSize() const {
  // Return a size of (0, 0) if the label is not visible and if the
  // |collapse_when_hidden_| flag is set.
  // TODO(munjal): This logic probably belongs to the View class. But for now,
  // put it here since putting it in View class means all inheriting classes
  // need to respect the |collapse_when_hidden_| flag.
  if (!visible() && collapse_when_hidden_)
    return gfx::Size();

  if (multi_line() && fixed_width_ != 0 && !text().empty())
    return gfx::Size(fixed_width_, GetHeightForWidth(fixed_width_));

  gfx::Size size(GetTextSize());
  const gfx::Insets insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());

  if (multi_line() && max_width_ != 0 && max_width_ < size.width())
    return gfx::Size(max_width_, GetHeightForWidth(max_width_));

  if (multi_line() && max_lines() > 0)
    return gfx::Size(size.width(), GetHeightForWidth(size.width()));
  return size;
}

gfx::Size Label::GetMinimumSize() const {
  if (!visible() && collapse_when_hidden_)
    return gfx::Size();

  gfx::Size size(0, font_list().GetHeight());
  if (elide_behavior_ == gfx::ELIDE_HEAD ||
      elide_behavior_ == gfx::ELIDE_MIDDLE ||
      elide_behavior_ == gfx::ELIDE_TAIL ||
      elide_behavior_ == gfx::ELIDE_EMAIL) {
    size.set_width(gfx::Canvas::GetStringWidth(
        base::string16(gfx::kEllipsisUTF16), font_list()));
  }

  if (!multi_line()) {
    if (elide_behavior_ == gfx::NO_ELIDE) {
      // If elision is disabled on single-line Labels, use text size as minimum.
      // This is OK because clients can use |gfx::ElideBehavior::TRUNCATE|
      // to get a non-eliding Label that should size itself less aggressively.
      size.SetToMax(GetTextSize());
    } else {
      size.SetToMin(GetTextSize());
    }
  }
  size.Enlarge(GetInsets().width(), GetInsets().height());
  return size;
}

int Label::GetHeightForWidth(int w) const {
  if (!visible() && collapse_when_hidden_)
    return 0;

  w -= GetInsets().width();
  int height = 0;
  int base_line_height = std::max(line_height(), font_list().GetHeight());
  if (!multi_line() || text().empty() || w <= 0) {
    height = base_line_height;
  } else {
    // SetDisplayRect() has a side effect for later calls of GetStringSize().
    // Be careful to invoke |full_text_->SetDisplayRect(gfx::Rect())| to
    // cancel this effect before the next time GetStringSize() is called.
    // It would be beneficial not to cancel here, considering that some layout
    // managers invoke GetHeightForWidth() for the same width multiple times
    // and |full_text_| can cache the height.
    full_text_->SetDisplayRect(gfx::Rect(0, 0, w, 0));
    int string_height = full_text_->GetStringSize().height();
    // Cap the number of lines to |max_lines()| if multi-line and non-zero
    // |max_lines()|.
    height = multi_line() && max_lines() > 0
                 ? std::min(max_lines() * base_line_height, string_height)
                 : string_height;
  }
  height -= gfx::ShadowValue::GetMargin(full_text_->shadows()).height();
  return height + GetInsets().height();
}

void Label::Layout() {
  ClearDisplayText();
}

const char* Label::GetClassName() const {
  return kViewClassName;
}

View* Label::GetTooltipHandlerForPoint(const gfx::Point& point) {
  if (!handles_tooltips_ ||
      (tooltip_text_.empty() && !ShouldShowDefaultTooltip()))
    return nullptr;

  return HitTestPoint(point) ? this : nullptr;
}

bool Label::CanProcessEventsWithinSubtree() const {
  return !!GetRenderTextForSelectionController();
}

WordLookupClient* Label::GetWordLookupClient() {
  return this;
}

void Label::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  if (text_context_ == style::CONTEXT_DIALOG_TITLE)
    node_data->role = ax::mojom::Role::kTitleBar;
  else
    node_data->role = ax::mojom::Role::kStaticText;

  node_data->SetName(full_text_->GetDisplayText());
}

bool Label::GetTooltipText(const gfx::Point& p, base::string16* tooltip) const {
  if (!handles_tooltips_)
    return false;

  if (!tooltip_text_.empty()) {
    tooltip->assign(tooltip_text_);
    return true;
  }

  if (ShouldShowDefaultTooltip()) {
    tooltip->assign(full_text_->GetDisplayText());
    return true;
  }

  return false;
}

std::unique_ptr<gfx::RenderText> Label::CreateRenderText() const {
  // Multi-line labels only support NO_ELIDE and ELIDE_TAIL for now.
  // TODO(warx): Investigate more elide text support.
  gfx::ElideBehavior elide_behavior =
      multi_line() && (elide_behavior_ != gfx::NO_ELIDE) ? gfx::ELIDE_TAIL
                                                         : elide_behavior_;

  auto render_text = gfx::RenderText::CreateHarfBuzzInstance();
  render_text->SetHorizontalAlignment(horizontal_alignment());
  render_text->SetDirectionalityMode(full_text_->directionality_mode());
  render_text->SetElideBehavior(elide_behavior);
  render_text->SetObscured(obscured());
  render_text->SetMinLineHeight(line_height());
  render_text->SetFontList(font_list());
  render_text->set_shadows(shadows());
  render_text->SetCursorEnabled(false);
  render_text->SetText(text());
  render_text->SetMultiline(multi_line());
  render_text->SetMaxLines(multi_line() ? max_lines() : 0);
  render_text->SetWordWrapBehavior(full_text_->word_wrap_behavior());

  // Setup render text for selection controller.
  if (selectable()) {
    render_text->set_focused(HasFocus());
    if (stored_selection_range_.IsValid())
      render_text->SelectRange(stored_selection_range_);
  }

  return render_text;
}

void Label::PaintFocusRing(gfx::Canvas* canvas) const {
  // No focus ring by default.
}

gfx::Rect Label::GetFocusRingBounds() const {
  MaybeBuildDisplayText();

  gfx::Rect focus_bounds;
  if (!display_text_) {
    focus_bounds = gfx::Rect(GetTextSize());
  } else {
    focus_bounds = gfx::Rect(gfx::Point() + display_text_->GetLineOffset(0),
                             display_text_->GetStringSize());
  }

  focus_bounds.Inset(-NonBorderInsets(*this));
  focus_bounds.Intersect(GetLocalBounds());
  return focus_bounds;
}

void Label::PaintText(gfx::Canvas* canvas) {
  MaybeBuildDisplayText();

  if (display_text_)
    display_text_->Draw(canvas);

#if DCHECK_IS_ON()
  // Attempt to ensure that if we're using subpixel rendering, we're painting
  // to an opaque background. What we don't want to find is an ancestor in the
  // hierarchy that paints to a non-opaque layer.
  if (!display_text_ || display_text_->subpixel_rendering_suppressed())
    return;

  for (View* view = this; view; view = view->parent()) {
    if (view->background() &&
        SkColorGetA(view->background()->get_color()) == SK_AlphaOPAQUE)
      break;

    if (view->layer() && view->layer()->fills_bounds_opaquely()) {
      DLOG(WARNING) << "Ancestor view has a non-opaque layer: "
                    << view->GetClassName() << " with ID " << view->id();
      break;
    }
  }
#endif
}

void Label::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  if (previous_bounds.size() != size())
    InvalidateLayout();
}

void Label::OnPaint(gfx::Canvas* canvas) {
  View::OnPaint(canvas);
  PaintText(canvas);
  if (HasFocus())
    PaintFocusRing(canvas);
}

void Label::OnNativeThemeChanged(const ui::NativeTheme* theme) {
  UpdateColorsFromTheme(theme);
}

gfx::NativeCursor Label::GetCursor(const ui::MouseEvent& event) {
  return GetRenderTextForSelectionController() ? GetNativeIBeamCursor()
                                               : gfx::kNullCursor;
}

void Label::OnFocus() {
  gfx::RenderText* render_text = GetRenderTextForSelectionController();
  if (render_text) {
    render_text->set_focused(true);
    SchedulePaint();
  }
  View::OnFocus();
}

void Label::OnBlur() {
  gfx::RenderText* render_text = GetRenderTextForSelectionController();
  if (render_text) {
    render_text->set_focused(false);
    SchedulePaint();
  }
  View::OnBlur();
}

bool Label::OnMousePressed(const ui::MouseEvent& event) {
  if (!GetRenderTextForSelectionController())
    return false;

  const bool had_focus = HasFocus();

  // RequestFocus() won't work when the label has FocusBehavior::NEVER. Hence
  // explicitly set the focused view.
  // TODO(karandeepb): If a widget with a label having FocusBehavior::NEVER as
  // the currently focused view (due to selection) was to lose focus, focus
  // won't be restored to the label (and hence a text selection won't be drawn)
  // when the widget gets focus again. Fix this.
  // Tracked in https://crbug.com/630365.
  if ((event.IsOnlyLeftMouseButton() || event.IsOnlyRightMouseButton()) &&
      GetFocusManager() && !had_focus) {
    GetFocusManager()->SetFocusedView(this);
  }

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  if (event.IsOnlyMiddleMouseButton() && GetFocusManager() && !had_focus)
    GetFocusManager()->SetFocusedView(this);
#endif

  return selection_controller_->OnMousePressed(
      event, false, had_focus ? SelectionController::FOCUSED
                              : SelectionController::UNFOCUSED);
}

bool Label::OnMouseDragged(const ui::MouseEvent& event) {
  if (!GetRenderTextForSelectionController())
    return false;

  return selection_controller_->OnMouseDragged(event);
}

void Label::OnMouseReleased(const ui::MouseEvent& event) {
  if (!GetRenderTextForSelectionController())
    return;

  selection_controller_->OnMouseReleased(event);
}

void Label::OnMouseCaptureLost() {
  if (!GetRenderTextForSelectionController())
    return;

  selection_controller_->OnMouseCaptureLost();
}

bool Label::OnKeyPressed(const ui::KeyEvent& event) {
  if (!GetRenderTextForSelectionController())
    return false;

  const bool shift = event.IsShiftDown();
  const bool control = event.IsControlDown();
  const bool alt = event.IsAltDown() || event.IsAltGrDown();

  switch (event.key_code()) {
    case ui::VKEY_C:
      if (control && !alt && HasSelection()) {
        CopyToClipboard();
        return true;
      }
      break;
    case ui::VKEY_INSERT:
      if (control && !shift && HasSelection()) {
        CopyToClipboard();
        return true;
      }
      break;
    case ui::VKEY_A:
      if (control && !alt && !text().empty()) {
        SelectAll();
        DCHECK(HasSelection());
        UpdateSelectionClipboard();
        return true;
      }
      break;
    default:
      break;
  }

  return false;
}

bool Label::AcceleratorPressed(const ui::Accelerator& accelerator) {
  // Allow the "Copy" action from the Chrome menu to be invoked. E.g., if a user
  // selects a Label on a web modal dialog. "Select All" doesn't appear in the
  // Chrome menu so isn't handled here.
  if (accelerator.key_code() == ui::VKEY_C && accelerator.IsCtrlDown()) {
    CopyToClipboard();
    return true;
  }
  return false;
}

bool Label::CanHandleAccelerators() const {
  // Focus needs to be checked since the accelerator for the Copy command from
  // the Chrome menu should only be handled when the current view has focus. See
  // related comment in BrowserView::CutCopyPaste.
  return HasFocus() && GetRenderTextForSelectionController() &&
         View::CanHandleAccelerators();
}

void Label::OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                       float new_device_scale_factor) {
  View::OnDeviceScaleFactorChanged(old_device_scale_factor,
                                   new_device_scale_factor);
  // When the device scale factor is changed, some font rendering parameters is
  // changed (especially, hinting). The bounding box of the text has to be
  // re-computed based on the new parameters. See crbug.com/441439
  ResetLayout();
}

void Label::VisibilityChanged(View* starting_from, bool is_visible) {
  if (!is_visible)
    ClearDisplayText();
}

void Label::ShowContextMenuForView(View* source,
                                   const gfx::Point& point,
                                   ui::MenuSourceType source_type) {
  if (!GetRenderTextForSelectionController())
    return;

  context_menu_runner_.reset(
      new MenuRunner(&context_menu_contents_,
                     MenuRunner::HAS_MNEMONICS | MenuRunner::CONTEXT_MENU));
  context_menu_runner_->RunMenuAt(GetWidget(), nullptr,
                                  gfx::Rect(point, gfx::Size()),
                                  MENU_ANCHOR_TOPLEFT, source_type);
}

bool Label::GetWordLookupDataAtPoint(const gfx::Point& point,
                                     gfx::DecoratedText* decorated_word,
                                     gfx::Point* baseline_point) {
  gfx::RenderText* render_text = GetRenderTextForSelectionController();
  return render_text ? render_text->GetWordLookupDataAtPoint(
                           point, decorated_word, baseline_point)
                     : false;
}

bool Label::GetWordLookupDataFromSelection(gfx::DecoratedText* decorated_text,
                                           gfx::Point* baseline_point) {
  gfx::RenderText* render_text = GetRenderTextForSelectionController();
  return render_text
             ? render_text->GetLookupDataForRange(
                   render_text->selection(), decorated_text, baseline_point)
             : false;
}

gfx::RenderText* Label::GetRenderTextForSelectionController() {
  return const_cast<gfx::RenderText*>(
      static_cast<const Label*>(this)->GetRenderTextForSelectionController());
}

bool Label::IsReadOnly() const {
  return true;
}

bool Label::SupportsDrag() const {
  // TODO(crbug.com/661379): Labels should support dragging selected text.
  return false;
}

bool Label::HasTextBeingDragged() const {
  return false;
}

void Label::SetTextBeingDragged(bool value) {
  NOTREACHED();
}

int Label::GetViewHeight() const {
  return height();
}

int Label::GetViewWidth() const {
  return width();
}

int Label::GetDragSelectionDelay() const {
  // Labels don't need to use a repeating timer to update the drag selection.
  // Since the cursor is disabled for labels, a selection outside the display
  // area won't change the text in the display area. It is expected that all the
  // text will fit in the display area for labels anyway.
  return 0;
}

void Label::OnBeforePointerAction() {}

void Label::OnAfterPointerAction(bool text_changed, bool selection_changed) {
  DCHECK(!text_changed);
  if (selection_changed)
    SchedulePaint();
}

bool Label::PasteSelectionClipboard() {
  NOTREACHED();
  return false;
}

void Label::UpdateSelectionClipboard() {
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  if (!obscured()) {
    ui::ScopedClipboardWriter(ui::CLIPBOARD_TYPE_SELECTION)
        .WriteText(GetSelectedText());
  }
#endif
}

bool Label::IsCommandIdChecked(int command_id) const {
  return true;
}

bool Label::IsCommandIdEnabled(int command_id) const {
  switch (command_id) {
    case IDS_APP_COPY:
      return HasSelection() && !obscured();
    case IDS_APP_SELECT_ALL:
      return GetRenderTextForSelectionController() && !text().empty();
  }
  return false;
}

void Label::ExecuteCommand(int command_id, int event_flags) {
  switch (command_id) {
    case IDS_APP_COPY:
      CopyToClipboard();
      break;
    case IDS_APP_SELECT_ALL:
      SelectAll();
      DCHECK(HasSelection());
      UpdateSelectionClipboard();
      break;
    default:
      NOTREACHED();
  }
}

bool Label::GetAcceleratorForCommandId(int command_id,
                                       ui::Accelerator* accelerator) const {
  switch (command_id) {
    case IDS_APP_COPY:
      *accelerator = ui::Accelerator(ui::VKEY_C, ui::EF_CONTROL_DOWN);
      return true;

    case IDS_APP_SELECT_ALL:
      *accelerator = ui::Accelerator(ui::VKEY_A, ui::EF_CONTROL_DOWN);
      return true;

    default:
      return false;
  }
}

const gfx::RenderText* Label::GetRenderTextForSelectionController() const {
  if (!selectable())
    return nullptr;
  MaybeBuildDisplayText();

  // This may be null when the content bounds of the view are empty.
  return display_text_.get();
}

void Label::Init(const base::string16& text, const gfx::FontList& font_list) {
  full_text_ = gfx::RenderText::CreateHarfBuzzInstance();
  DCHECK(full_text_->MultilineSupported());
  full_text_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  full_text_->SetDirectionalityMode(gfx::DIRECTIONALITY_FROM_TEXT);
  // NOTE: |full_text_| should not be elided at all. This is used to keep
  // some properties and to compute the size of the string.
  full_text_->SetElideBehavior(gfx::NO_ELIDE);
  full_text_->SetFontList(font_list);
  full_text_->SetCursorEnabled(false);
  full_text_->SetWordWrapBehavior(gfx::TRUNCATE_LONG_WORDS);

  elide_behavior_ = gfx::ELIDE_TAIL;
  stored_selection_range_ = gfx::Range::InvalidRange();
  enabled_color_set_ = background_color_set_ = false;
  selection_text_color_set_ = selection_background_color_set_ = false;
  subpixel_rendering_enabled_ = true;
  auto_color_readability_ = true;
  multi_line_ = false;
  max_lines_ = 0;
  UpdateColorsFromTheme(GetNativeTheme());
  handles_tooltips_ = true;
  collapse_when_hidden_ = false;
  fixed_width_ = 0;
  max_width_ = 0;
  SetText(text);

  // Only selectable labels will get requests to show the context menu, due to
  // CanProcessEventsWithinSubtree().
  BuildContextMenuContents();
  set_context_menu_controller(this);

  // This allows the BrowserView to pass the copy command from the Chrome menu
  // to the Label.
  AddAccelerator(ui::Accelerator(ui::VKEY_C, ui::EF_CONTROL_DOWN));
}

void Label::ResetLayout() {
  InvalidateLayout();
  PreferredSizeChanged();
  SchedulePaint();
  ClearDisplayText();
}

void Label::MaybeBuildDisplayText() const {
  if (display_text_)
    return;

  gfx::Rect rect = GetContentsBounds();
  rect.Inset(NonBorderInsets(*this));
  if (rect.IsEmpty())
    return;

  rect.Inset(-gfx::ShadowValue::GetMargin(shadows()));
  display_text_ = CreateRenderText();
  display_text_->SetDisplayRect(rect);
  stored_selection_range_ = gfx::Range::InvalidRange();
  ApplyTextColors();
}

gfx::Size Label::GetTextSize() const {
  gfx::Size size;
  if (text().empty()) {
    size = gfx::Size(0, std::max(line_height(), font_list().GetHeight()));
  } else {
    // Cancel the display rect of |full_text_|. The display rect may be
    // specified in GetHeightForWidth(), and specifying empty Rect cancels
    // its effect. See also the comment in GetHeightForWidth().
    // TODO(mukai): use gfx::Rect() to compute the ideal size rather than
    // the current width(). See crbug.com/468494, crbug.com/467526, and
    // the comment for MultilinePreferredSizeTest in label_unittest.cc.
    full_text_->SetDisplayRect(gfx::Rect(0, 0, width(), 0));
    size = full_text_->GetStringSize();
  }
  const gfx::Insets shadow_margin = -gfx::ShadowValue::GetMargin(shadows());
  size.Enlarge(shadow_margin.width(), shadow_margin.height());
  return size;
}

void Label::RecalculateColors() {
  actual_enabled_color_ = auto_color_readability_ ?
      color_utils::GetReadableColor(requested_enabled_color_,
                                    background_color_) :
      requested_enabled_color_;
  actual_selection_text_color_ =
      auto_color_readability_
          ? color_utils::GetReadableColor(requested_selection_text_color_,
                                          selection_background_color_)
          : requested_selection_text_color_;

  ApplyTextColors();
  SchedulePaint();
}

void Label::ApplyTextColors() const {
  if (!display_text_)
    return;

  bool subpixel_rendering_suppressed =
      SkColorGetA(background_color_) != SK_AlphaOPAQUE ||
      !subpixel_rendering_enabled_;
  display_text_->SetColor(actual_enabled_color_);
  display_text_->set_selection_color(actual_selection_text_color_);
  display_text_->set_selection_background_focused_color(
      selection_background_color_);
  display_text_->set_subpixel_rendering_suppressed(
      subpixel_rendering_suppressed);
}

void Label::UpdateColorsFromTheme(const ui::NativeTheme* theme) {
  if (!enabled_color_set_) {
    requested_enabled_color_ =
        style::GetColor(*this, text_context_, style::STYLE_PRIMARY);
  }
  if (!background_color_set_) {
    background_color_ =
        theme->GetSystemColor(ui::NativeTheme::kColorId_DialogBackground);
  }
  if (!selection_text_color_set_) {
    requested_selection_text_color_ = theme->GetSystemColor(
        ui::NativeTheme::kColorId_LabelTextSelectionColor);
  }
  if (!selection_background_color_set_) {
    selection_background_color_ = theme->GetSystemColor(
        ui::NativeTheme::kColorId_LabelTextSelectionBackgroundFocused);
  }
  RecalculateColors();
}

bool Label::ShouldShowDefaultTooltip() const {
  const gfx::Size text_size = GetTextSize();
  const gfx::Size size = GetContentsBounds().size();
  return !obscured() && (text_size.width() > size.width() ||
                         (multi_line() && text_size.height() > size.height()));
}

void Label::ClearDisplayText() const {
  // The HasSelection() call below will build |display_text_| in case it is
  // empty. Return early to avoid this.
  if (!display_text_)
    return;

  // Persist the selection range if there is an active selection.
  if (HasSelection()) {
    stored_selection_range_ =
        GetRenderTextForSelectionController()->selection();
  }
  display_text_ = nullptr;
}

base::string16 Label::GetSelectedText() const {
  const gfx::RenderText* render_text = GetRenderTextForSelectionController();
  return render_text ? render_text->GetTextFromRange(render_text->selection())
                     : base::string16();
}

void Label::CopyToClipboard() {
  if (!HasSelection() || obscured())
    return;
  ui::ScopedClipboardWriter(ui::CLIPBOARD_TYPE_COPY_PASTE)
      .WriteText(GetSelectedText());
}

void Label::BuildContextMenuContents() {
  context_menu_contents_.AddItemWithStringId(IDS_APP_COPY, IDS_APP_COPY);
  context_menu_contents_.AddItemWithStringId(IDS_APP_SELECT_ALL,
                                             IDS_APP_SELECT_ALL);
}

}  // namespace views
