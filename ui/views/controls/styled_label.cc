// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/styled_label.h"

#include <stddef.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

#include "base/i18n/rtl.h"
#include "base/strings/string_util.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/text_elider.h"
#include "ui/gfx/text_utils.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/styled_label_listener.h"

namespace views {

// Helpers --------------------------------------------------------------------

namespace {

gfx::Insets FocusBorderInsets(const Label& label) {
  // StyledLabel never adds a border, so the only Insets added are for the
  // possible focus ring.
  DCHECK(label.View::GetInsets().IsEmpty());
  return label.GetInsets();
}

std::unique_ptr<Label> CreateLabelRange(
    const base::string16& text,
    int text_context,
    int default_style,
    const StyledLabel::RangeStyleInfo& style_info,
    views::LinkListener* link_listener) {
  std::unique_ptr<Label> result;

  if (style_info.IsLink()) {
    // Nothing should (and nothing does) use a custom font for links.
    DCHECK(!style_info.custom_font);

    // Note this ignores |default_style|, in favor of style::STYLE_LINK.
    Link* link = new Link(text, text_context);
    link->set_listener(link_listener);

    // Links in a StyledLabel do not get underlines.
    link->SetUnderline(false);

    result.reset(link);
  } else if (style_info.custom_font) {
    result.reset(new Label(text, {style_info.custom_font.value()}));
  } else {
    result.reset(new Label(text, text_context,
                           style_info.text_style.value_or(default_style)));
  }
  if (style_info.override_color != SK_ColorTRANSPARENT)
    result->SetEnabledColor(style_info.override_color);

  if (!style_info.tooltip.empty())
    result->SetTooltipText(style_info.tooltip);

  return result;
}

// Returns the horizontal offset to align views in a line.
int HorizontalAdjustment(int used_width,
                         int width,
                         gfx::HorizontalAlignment alignment) {
  const int space = width - used_width;
  return alignment == gfx::ALIGN_LEFT
             ? 0
             : alignment == gfx::ALIGN_CENTER ? space / 2 : space;
}

}  // namespace

// StyledLabel::RangeStyleInfo ------------------------------------------------

StyledLabel::RangeStyleInfo::RangeStyleInfo() = default;
StyledLabel::RangeStyleInfo::RangeStyleInfo(const RangeStyleInfo& copy) =
    default;

StyledLabel::RangeStyleInfo::~RangeStyleInfo() {}

// static
StyledLabel::RangeStyleInfo StyledLabel::RangeStyleInfo::CreateForLink() {
  RangeStyleInfo result;
  result.disable_line_wrapping = true;
  result.text_style = style::STYLE_LINK;
  return result;
}

bool StyledLabel::RangeStyleInfo::IsLink() const {
  return text_style && text_style.value() == style::STYLE_LINK;
}

// StyledLabel::StyleRange ----------------------------------------------------

bool StyledLabel::StyleRange::operator<(
    const StyledLabel::StyleRange& other) const {
  return range.start() < other.range.start();
}


// StyledLabel ----------------------------------------------------------------

// static
const char StyledLabel::kViewClassName[] = "StyledLabel";

StyledLabel::StyledLabel(const base::string16& text,
                         StyledLabelListener* listener)
    : specified_line_height_(0),
      listener_(listener),
      width_at_last_size_calculation_(0),
      width_at_last_layout_(0),
      displayed_on_background_color_(SkColorSetRGB(0xFF, 0xFF, 0xFF)),
      displayed_on_background_color_set_(false),
      auto_color_readability_enabled_(true) {
  base::TrimWhitespace(text, base::TRIM_TRAILING, &text_);
}

StyledLabel::~StyledLabel() {}

void StyledLabel::SetText(const base::string16& text) {
  text_ = text;
  style_ranges_.clear();
  RemoveAllChildViews(true);
  PreferredSizeChanged();
}

gfx::FontList StyledLabel::GetDefaultFontList() const {
  return style::GetFont(text_context_, default_text_style_);
}

void StyledLabel::AddStyleRange(const gfx::Range& range,
                                const RangeStyleInfo& style_info) {
  DCHECK(!range.is_reversed());
  DCHECK(!range.is_empty());
  DCHECK(gfx::Range(0, text_.size()).Contains(range));

  // Insert the new range in sorted order.
  StyleRanges new_range;
  new_range.push_front(StyleRange(range, style_info));
  style_ranges_.merge(new_range);

  PreferredSizeChanged();
}

void StyledLabel::AddCustomView(std::unique_ptr<View> custom_view) {
  DCHECK(custom_view->owned_by_client());
  custom_views_.insert(std::move(custom_view));
}

void StyledLabel::SetTextContext(int text_context) {
  if (text_context_ == text_context)
    return;

  text_context_ = text_context;
  PreferredSizeChanged();
}

void StyledLabel::SetDefaultTextStyle(int text_style) {
  if (default_text_style_ == text_style)
    return;

  default_text_style_ = text_style;
  PreferredSizeChanged();
}

void StyledLabel::SetLineHeight(int line_height) {
  specified_line_height_ = line_height;
  PreferredSizeChanged();
}

void StyledLabel::SetDisplayedOnBackgroundColor(SkColor color) {
  if (displayed_on_background_color_ == color &&
      displayed_on_background_color_set_)
    return;

  displayed_on_background_color_ = color;
  displayed_on_background_color_set_ = true;

  for (int i = 0, count = child_count(); i < count; ++i) {
    DCHECK((child_at(i)->GetClassName() == Label::kViewClassName) ||
           (child_at(i)->GetClassName() == Link::kViewClassName));
    static_cast<Label*>(child_at(i))->SetBackgroundColor(color);
  }
}

void StyledLabel::SizeToFit(int max_width) {
  if (max_width == 0)
    max_width = std::numeric_limits<int>::max();

  SetSize(CalculateAndDoLayout(max_width, true));
}

const char* StyledLabel::GetClassName() const {
  return kViewClassName;
}

gfx::Insets StyledLabel::GetInsets() const {
  gfx::Insets insets = View::GetInsets();
  if (Link::GetDefaultFocusStyle() != Link::FocusStyle::RING)
    return insets;

  // We need a focus border iff we contain a link that will have a focus border.
  // That in turn will be true only if the link is non-empty.
  for (StyleRanges::const_iterator i(style_ranges_.begin());
        i != style_ranges_.end(); ++i) {
    if (i->style_info.IsLink() && !i->range.is_empty()) {
      insets += gfx::Insets(Link::kFocusBorderPadding);
      break;
    }
  }

  return insets;
}

void StyledLabel::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  if (text_context_ == style::CONTEXT_DIALOG_TITLE)
    node_data->role = ax::mojom::Role::kTitleBar;
  else
    node_data->role = ax::mojom::Role::kStaticText;

  node_data->SetName(text());
}

gfx::Size StyledLabel::CalculatePreferredSize() const {
  return calculated_size_;
}

int StyledLabel::GetHeightForWidth(int w) const {
  // TODO(erg): Munge the const-ness of the style label. CalculateAndDoLayout
  // doesn't actually make any changes to member variables when |dry_run| is
  // set to true. In general, the mutating and non-mutating parts shouldn't
  // be in the same codepath.
  return const_cast<StyledLabel*>(this)->CalculateAndDoLayout(w, true).height();
}

void StyledLabel::Layout() {
  CalculateAndDoLayout(GetLocalBounds().width(), false);
}

void StyledLabel::PreferredSizeChanged() {
  calculated_size_ = gfx::Size();
  width_at_last_size_calculation_ = 0;
  width_at_last_layout_ = 0;
  View::PreferredSizeChanged();
}

void StyledLabel::LinkClicked(Link* source, int event_flags) {
  if (listener_)
    listener_->StyledLabelLinkClicked(this, link_targets_[source], event_flags);
}

// TODO(wutao): support gfx::ALIGN_TO_HEAD alignment.
void StyledLabel::SetHorizontalAlignment(gfx::HorizontalAlignment alignment) {
  DCHECK_NE(gfx::ALIGN_TO_HEAD, alignment);
  alignment = gfx::MaybeFlipForRTL(alignment);

  if (horizontal_alignment_ == alignment)
    return;
  horizontal_alignment_ = alignment;
  PreferredSizeChanged();
}

void StyledLabel::ClearStyleRanges() {
  style_ranges_.clear();
  PreferredSizeChanged();
}

int StyledLabel::GetDefaultLineHeight() const {
  return specified_line_height_ > 0
             ? specified_line_height_
             : std::max(
                   style::GetLineHeight(text_context_, default_text_style_),
                   GetDefaultFontList().GetHeight());
}

gfx::FontList StyledLabel::GetFontListForRange(
    const StyleRanges::const_iterator& range) const {
  if (range == style_ranges_.end())
    return GetDefaultFontList();

  return range->style_info.custom_font
             ? range->style_info.custom_font.value()
             : style::GetFont(
                   text_context_,
                   range->style_info.text_style.value_or(default_text_style_));
}

gfx::Size StyledLabel::CalculateAndDoLayout(int width, bool dry_run) {
  if (width == width_at_last_size_calculation_ &&
      (dry_run || width == width_at_last_layout_))
    return calculated_size_;

  width_at_last_size_calculation_ = width;
  if (!dry_run)
    width_at_last_layout_ = width;

  width -= GetInsets().width();

  if (!dry_run) {
    RemoveAllChildViews(true);
    link_targets_.clear();
  }

  if (width <= 0 || text_.empty())
    return gfx::Size();

  const int default_line_height = GetDefaultLineHeight();

  // The index of the line we're on.
  int line = 0;
  const gfx::Insets insets = GetInsets();
  // The current child view's position, relative to content bounds, in pixels.
  gfx::Point offset(0, insets.top());
  int total_height = 0;
  // The width that was actually used. Guaranteed to be no larger than |width|.
  int used_width = 0;

  RangeStyleInfo default_style;
  default_style.text_style = default_text_style_;

  base::string16 remaining_string = text_;
  StyleRanges::const_iterator current_range = style_ranges_.begin();

  bool first_loop_iteration = true;

  // Max height of the views in a line.
  int max_line_height = default_line_height;

  // Temporary references to the views in a line, used for alignment.
  std::vector<View*> views_in_a_line;

  // Iterate over the text, creating a bunch of labels and links and laying them
  // out in the appropriate positions.
  while (!remaining_string.empty()) {
    if (offset.x() == 0 && !first_loop_iteration) {
      if (remaining_string.front() == L'\n') {
        // Wrapped to the next line on \n, remove it. Other whitespace,
        // eg, spaces to indent next line, are preserved.
        remaining_string.erase(0, 1);
      } else {
        // Wrapped on whitespace character or characters in the middle of the
        // line - none of them are needed at the beginning of the next line.
        base::TrimWhitespace(remaining_string, base::TRIM_LEADING,
                             &remaining_string);
      }
    }
    first_loop_iteration = false;

    gfx::Range range(gfx::Range::InvalidRange());
    if (current_range != style_ranges_.end())
      range = current_range->range;

    const size_t position = text_.size() - remaining_string.size();
    std::vector<base::string16> substrings;
    // If the current range is not a custom_view, then we use ElideRectangleText
    // to determine the line wrapping. Note: if it is a custom_view, then the
    // |position| should equal |range.start()| because the custom_view is
    // treated as one unit.
    if (position != range.start() || (current_range != style_ranges_.end() &&
                                      !current_range->style_info.custom_view)) {
      const gfx::Rect chunk_bounds(offset.x(), 0, width - offset.x(),
                                   default_line_height);
      // If the start of the remaining text is inside a styled range, the font
      // style may differ from the base font. The font specified by the range
      // should be used when eliding text.
      gfx::FontList text_font_list = position >= range.start()
                                         ? GetFontListForRange(current_range)
                                         : GetDefaultFontList();
      int elide_result = gfx::ElideRectangleText(
          remaining_string, text_font_list, chunk_bounds.width(),
          chunk_bounds.height(), gfx::WRAP_LONG_WORDS, &substrings);

      if (substrings.empty()) {
        // There is no room for anything; abort. Since wrapping is enabled, this
        // should only occur if there is insufficient vertical space remaining.
        // ElideRectangleText always adds a single character, even if there is
        // no room horizontally.
        DCHECK_NE(0, elide_result & gfx::INSUFFICIENT_SPACE_VERTICAL);
        break;
      }

      // Views are aligned to integer coordinates, but typesetting is not. This
      // means that it's possible for an ElideRectangleText on a prior iteration
      // to fit a word on the current line, which does not fit after that word
      // is wrapped in a View for its chunk at the end of the line. In most
      // cases, this will just wrap more words on to the next line. However, if
      // the remaining chunk width is insufficient for the very _first_ word,
      // that word will be incorrectly split. In this case, start a new line
      // instead.
      bool truncated_chunk =
          offset.x() != 0 &&
          (elide_result & gfx::INSUFFICIENT_SPACE_FOR_FIRST_WORD) != 0;
      if (substrings[0].empty() || truncated_chunk) {
        // The entire line is \n, or nothing fits on this line. Start a new
        // line. As for the first line, don't advance line number so that it
        // will be handled again at the beginning of the loop.
        AdvanceOneLine(&line, &offset, &max_line_height, width,
                       &views_in_a_line,
                       offset.x() != 0 || line > 0 /* new_line */);
        continue;
      }
    }

    base::string16 chunk;
    View* custom_view = nullptr;
    std::unique_ptr<Label> label;
    if (position >= range.start()) {
      const RangeStyleInfo& style_info = current_range->style_info;

      if (style_info.custom_view) {
        custom_view = style_info.custom_view;
        // Ownership of the custom view must be passed to StyledLabel.
        DCHECK(
            std::find_if(custom_views_.cbegin(), custom_views_.cend(),
                         [custom_view](const std::unique_ptr<View>& view_ptr) {
                           return view_ptr.get() == custom_view;
                         }) != custom_views_.cend());
        // Do not allow wrap in custom view.
        DCHECK_EQ(position, range.start());
        chunk = remaining_string.substr(0, range.end() - position);
      } else {
        chunk = substrings[0];
      }

      if (((custom_view &&
            offset.x() + custom_view->GetPreferredSize().width() > width) ||
           (style_info.disable_line_wrapping &&
            chunk.size() < range.length())) &&
          position == range.start() && offset.x() != 0) {
        // If the chunk should not be wrapped, try to fit it entirely on the
        // next line.
        AdvanceOneLine(&line, &offset, &max_line_height, width,
                       &views_in_a_line);
        continue;
      }

      if (chunk.size() > range.end() - position)
        chunk = chunk.substr(0, range.end() - position);

      if (!custom_view) {
        label = CreateLabelRange(chunk, text_context_, default_text_style_,
                                 style_info, this);
        if (style_info.IsLink() && !dry_run)
          link_targets_[label.get()] = range;
      }

      if (position + chunk.size() >= range.end())
        ++current_range;
    } else {
      chunk = substrings[0];

      // This chunk is normal text.
      if (position + chunk.size() > range.start())
        chunk = chunk.substr(0, range.start() - position);
      label = CreateLabelRange(chunk, text_context_, default_text_style_,
                               default_style, this);
    }

    if (label) {
      if (displayed_on_background_color_set_)
        label->SetBackgroundColor(displayed_on_background_color_);
      label->SetAutoColorReadabilityEnabled(auto_color_readability_enabled_);
    }

    View* child_view = custom_view ? custom_view : label.get();
    gfx::Size view_size = child_view->GetPreferredSize();
    // |offset.y()| already contains |insets.top()|.
    gfx::Point view_origin(insets.left() + offset.x(), offset.y());
    gfx::Insets focus_border_insets;
    if (Link::GetDefaultFocusStyle() == Link::FocusStyle::RING && label) {
      // Calculate the size of the optional focus border, and overlap by that
      // amount. Otherwise, "<a>link</a>," will render as "link ,".
      focus_border_insets = FocusBorderInsets(*label);
    }
    view_origin.Offset(-focus_border_insets.left(), -focus_border_insets.top());
    // The custom view could be wider than the available width; clamp as needed.
    if (custom_view) {
      view_size.set_width(std::min(
          view_size.width(), width - offset.x() + focus_border_insets.width()));
    }
    child_view->SetBoundsRect(gfx::Rect(view_origin, view_size));
    offset.set_x(offset.x() + view_size.width() - focus_border_insets.width());
    total_height =
        std::max(total_height, child_view->bounds().bottom() + insets.bottom() -
                                   focus_border_insets.bottom());
    used_width = std::max(used_width, offset.x());
    max_line_height = std::max(
        max_line_height, view_size.height() - focus_border_insets.height());

    if (!dry_run) {
      views_in_a_line.push_back(child_view);
      if (label)
        AddChildView(label.release());
      else
        AddChildView(child_view);
    }

    // If |gfx::ElideRectangleText| returned more than one substring, that
    // means the whole text did not fit into remaining line width, with text
    // after |susbtring[0]| spilling into next line. If whole |substring[0]|
    // was added to the current line (this may not be the case if part of the
    // substring has different style), proceed to the next line.
    if (!custom_view && substrings.size() > 1 &&
        chunk.size() == substrings[0].size()) {
      AdvanceOneLine(&line, &offset, &max_line_height, width, &views_in_a_line);
    }

    remaining_string = remaining_string.substr(chunk.size());
  }
  AdvanceOneLine(&line, &offset, &max_line_height, width, &views_in_a_line,
                 false);
  DCHECK_LE(used_width, width);
  calculated_size_ = gfx::Size(used_width + GetInsets().width(), total_height);
  return calculated_size_;
}

void StyledLabel::AdvanceOneLine(int* line_number,
                                 gfx::Point* offset,
                                 int* max_line_height,
                                 int width,
                                 std::vector<View*>* views_in_a_line,
                                 bool new_line) {
  const int x_delta =
      HorizontalAdjustment(offset->x(), width, horizontal_alignment_);
  for (auto* view : *views_in_a_line) {
    gfx::Rect bounds = view->bounds();
    bounds.set_x(bounds.x() + x_delta);
    bounds.set_y(offset->y() + (*max_line_height - bounds.height()) / 2.0f);
    view->SetBoundsRect(bounds);
  }
  views_in_a_line->clear();

  if (new_line) {
    ++(*line_number);
    offset->set_y(offset->y() + *max_line_height);
    *max_line_height = GetDefaultLineHeight();
  }
  offset->set_x(0);
}

}  // namespace views
