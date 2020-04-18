// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_STYLED_LABEL_H_
#define UI_VIEWS_CONTROLS_STYLED_LABEL_H_

#include <list>
#include <map>
#include <memory>
#include <set>

#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/range/range.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"

namespace views {

class Link;
class StyledLabelListener;

// A class which can apply mixed styles to a block of text. Currently, text is
// always multiline. Trailing whitespace in the styled label text is not
// supported and will be trimmed on StyledLabel construction. Leading
// whitespace is respected, provided not only whitespace fits in the first line.
// In this case, leading whitespace is ignored.
class VIEWS_EXPORT StyledLabel : public View, public LinkListener {
 public:
  // Internal class name.
  static const char kViewClassName[];

  // Parameters that define label style for a styled label's text range.
  struct VIEWS_EXPORT RangeStyleInfo {
    RangeStyleInfo();
    RangeStyleInfo(const RangeStyleInfo& copy);
    ~RangeStyleInfo();

    // Creates a range style info with default values for link.
    static RangeStyleInfo CreateForLink();

    bool IsLink() const;

    // Allows full customization of the font used in the range. Ignores the
    // StyledLabel's default text context and |text_style|.
    base::Optional<gfx::FontList> custom_font;

    // The style::TextStyle for this range.
    base::Optional<int> text_style;

    // Overrides the text color given by |text_style| for this range. Default is
    // SK_ColorTRANSPARENT, indicating not to override.
    // DEPRECATED: Use TextStyle.
    SkColor override_color = SK_ColorTRANSPARENT;

    // Tooltip for the range.
    base::string16 tooltip;

    // If set, the whole range will be put on a single line.
    bool disable_line_wrapping = false;

    // A custom view shown instead of the underlying text. Ownership of custom
    // views must be passed to StyledLabel via AddCustomView().
    View* custom_view = nullptr;
  };

  // Note that any trailing whitespace in |text| will be trimmed.
  StyledLabel(const base::string16& text, StyledLabelListener* listener);
  ~StyledLabel() override;

  // Sets the text to be displayed, and clears any previous styling.
  void SetText(const base::string16& text);

  const base::string16& text() const { return text_; }

  // Returns the font list that results from the default text context and style
  // for ranges. This can be used as the basis for a range |custom_font|.
  gfx::FontList GetDefaultFontList() const;

  // Marks the given range within |text_| with style defined by |style_info|.
  // |range| must be contained in |text_|.
  void AddStyleRange(const gfx::Range& range, const RangeStyleInfo& style_info);

  // Passes ownership of a custom view for use by RangeStyleInfo structs.
  void AddCustomView(std::unique_ptr<View> custom_view);

  // Set the context of this text. All ranges have the same context.
  // |text_context| must be a value from views::style::TextContext.
  void SetTextContext(int text_context);

  // Set the default text style.
  // |text_style| must be a value from views::style::TextStyle.
  void SetDefaultTextStyle(int text_style);

  // Get or set the distance in pixels between baselines of multi-line text.
  // Default is 0, indicating the distance between lines should be the standard
  // one for the label's text, font list, and platform.
  void SetLineHeight(int height);

  // Sets the color of the background on which the label is drawn. This won't
  // be explicitly drawn, but the label will force the text color to be
  // readable over it.
  void SetDisplayedOnBackgroundColor(SkColor color);
  SkColor displayed_on_background_color() const {
    return displayed_on_background_color_;
  }

  void set_auto_color_readability_enabled(bool auto_color_readability) {
    auto_color_readability_enabled_ = auto_color_readability;
  }

  // Resizes the label so its width is set to the width of the longest line and
  // its height deduced accordingly.
  // This is only intended for multi-line labels and is useful when the label's
  // text contains several lines separated with \n.
  // |max_width| is the maximum width that will be used (longer lines will be
  // wrapped). If 0, no maximum width is enforced.
  void SizeToFit(int max_width);

  // View:
  const char* GetClassName() const override;
  gfx::Insets GetInsets() const override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  gfx::Size CalculatePreferredSize() const override;
  int GetHeightForWidth(int w) const override;
  void Layout() override;
  void PreferredSizeChanged() override;

  // LinkListener implementation:
  void LinkClicked(Link* source, int event_flags) override;

  // Sets the horizontal alignment; the argument value is mirrored in RTL UI.
  void SetHorizontalAlignment(gfx::HorizontalAlignment alignment);

  // Clears all the styles applied to the label.
  void ClearStyleRanges();

 private:
  struct StyleRange {
    StyleRange(const gfx::Range& range,
               const RangeStyleInfo& style_info)
        : range(range),
          style_info(style_info) {
    }
    ~StyleRange() {}

    bool operator<(const StyleRange& other) const;

    gfx::Range range;
    RangeStyleInfo style_info;
  };
  typedef std::list<StyleRange> StyleRanges;

  // Returns the default line height, based on the default style.
  int GetDefaultLineHeight() const;

  // Returns the FontList that should be used for |range|.
  gfx::FontList GetFontListForRange(
      const StyleRanges::const_iterator& range) const;

  // Calculates how to layout child views, creates them and sets their size and
  // position. |width| is the horizontal space, in pixels, that the view has to
  // work with. If |dry_run| is true, the view hierarchy is not touched. Caches
  // the results in |calculated_size_|, |width_at_last_layout_|, and
  // |width_at_last_size_calculation_|. Returns the needed size.
  gfx::Size CalculateAndDoLayout(int width, bool dry_run);

  // Adjusts the offsets of the views in a line for alignment and other line
  // parameters.
  void AdvanceOneLine(int* line_number,
                      gfx::Point* offset,
                      int* max_line_height,
                      int width,
                      std::vector<View*>* views_in_a_line,
                      bool new_line = true);

  // The text to display.
  base::string16 text_;

  int text_context_ = style::CONTEXT_LABEL;
  int default_text_style_ = style::STYLE_PRIMARY;

  // Line height. If zero, style::GetLineHeight() is used.
  int specified_line_height_;

  // The listener that will be informed of link clicks.
  StyledLabelListener* listener_;

  // The ranges that should be linkified, sorted by start position.
  StyleRanges style_ranges_;

  // A mapping from a view to the range it corresponds to in |text_|. Only views
  // that correspond to ranges with is_link style set will be added to the map.
  std::map<View*, gfx::Range> link_targets_;

  // Owns the custom views used to replace ranges of text with icons, etc.
  std::set<std::unique_ptr<View>> custom_views_;

  // This variable saves the result of the last GetHeightForWidth call in order
  // to avoid repeated calculation.
  mutable gfx::Size calculated_size_;
  mutable int width_at_last_size_calculation_;
  int width_at_last_layout_;

  // Background color on which the label is drawn, for auto color readability.
  SkColor displayed_on_background_color_;
  bool displayed_on_background_color_set_;

  // Controls whether the text is automatically re-colored to be readable on the
  // background.
  bool auto_color_readability_enabled_;

  // The horizontal alignment. This value is flipped for RTL. The default
  // behavior is to align left in LTR UI and right in RTL UI.
  gfx::HorizontalAlignment horizontal_alignment_ = gfx::ALIGN_LEFT;

  DISALLOW_COPY_AND_ASSIGN(StyledLabel);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_STYLED_LABEL_H_
