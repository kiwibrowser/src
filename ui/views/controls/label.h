// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_LABEL_H_
#define UI_VIEWS_CONTROLS_LABEL_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/render_text.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/selection_controller_delegate.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/word_lookup_client.h"

namespace views {
class LabelSelectionTest;
class MenuRunner;
class SelectionController;

// A view subclass that can display a string.
class VIEWS_EXPORT Label : public View,
                           public ContextMenuController,
                           public WordLookupClient,
                           public SelectionControllerDelegate,
                           public ui::SimpleMenuModel::Delegate {
 public:
  // Internal class name.
  static const char kViewClassName[];

  // Helper to construct a Label that doesn't use the views typography spec.
  // Using this causes Label to obtain colors from ui::NativeTheme and line
  // spacing from gfx::FontList::GetHeight().
  // TODO(tapted): Audit users of this class when MD is default. Then add
  // foreground/background colors, line spacing and everything else that
  // views::TextContext abstracts away so the separate setters can be removed.
  struct CustomFont {
    // TODO(tapted): Change this to a size delta and font weight since that's
    // typically all the callers really care about, and would allow Label to
    // guarantee caching of the FontList in ResourceBundle.
    const gfx::FontList& font_list;
  };

  // Create Labels with style::CONTEXT_CONTROL_LABEL and style::STYLE_PRIMARY.
  // TODO(tapted): Remove these. Callers must specify a context or use the
  // constructor taking a CustomFont.
  Label();
  explicit Label(const base::string16& text);

  // Construct a Label in the given |text_context|. The |text_style| can change
  // later, so provide a default. The |text_context| is fixed.
  Label(const base::string16& text,
        int text_context,
        int text_style = style::STYLE_PRIMARY);

  // Construct a Label with the given |font| description.
  Label(const base::string16& text, const CustomFont& font);

  ~Label() override;

  static const gfx::FontList& GetDefaultFontList();

  // Gets or sets the fonts used by this label.
  const gfx::FontList& font_list() const { return full_text_->font_list(); }

  // TODO(tapted): Replace this with a private method, e.g., OnFontChanged().
  virtual void SetFontList(const gfx::FontList& font_list);

  // Get or set the label text.
  const base::string16& text() const { return full_text_->text(); }
  virtual void SetText(const base::string16& text);

  // Where the label appears in the UI. Passed in from the constructor. This is
  // a value from views::style::TextContext or an enum that extends it.
  int text_context() const { return text_context_; }

  // Enables or disables auto-color-readability (enabled by default).  If this
  // is enabled, then calls to set any foreground or background color will
  // trigger an automatic mapper that uses color_utils::GetReadableColor() to
  // ensure that the foreground colors are readable over the background color.
  void SetAutoColorReadabilityEnabled(bool enabled);

  // Sets the color.  This will automatically force the color to be readable
  // over the current background color, if auto color readability is enabled.
  virtual void SetEnabledColor(SkColor color);

  SkColor enabled_color() const { return actual_enabled_color_; }

  // Sets the background color. This won't be explicitly drawn, but the label
  // will force the text color to be readable over it.
  void SetBackgroundColor(SkColor color);
  SkColor background_color() const { return background_color_; }

  // Sets the selection text color. This will automatically force the color to
  // be readable over the selection background color, if auto color readability
  // is enabled. Initialized with system default.
  void SetSelectionTextColor(SkColor color);
  SkColor selection_text_color() const { return actual_selection_text_color_; }

  // Sets the selection background color. Initialized with system default.
  void SetSelectionBackgroundColor(SkColor color);
  SkColor selection_background_color() const {
    return selection_background_color_;
  }

  // Set drop shadows underneath the text.
  void SetShadows(const gfx::ShadowValues& shadows);
  const gfx::ShadowValues& shadows() const { return full_text_->shadows(); }

  // Sets whether subpixel rendering is used; the default is true, but this
  // feature also requires an opaque background color.
  // TODO(mukai): rename this as SetSubpixelRenderingSuppressed() to keep the
  // consistency with RenderText field name.
  void SetSubpixelRenderingEnabled(bool subpixel_rendering_enabled);

  // Sets the horizontal alignment; the argument value is mirrored in RTL UI.
  void SetHorizontalAlignment(gfx::HorizontalAlignment alignment);
  gfx::HorizontalAlignment horizontal_alignment() const {
    return full_text_->horizontal_alignment();
  }

  // Get or set the distance in pixels between baselines of multi-line text.
  // Default is 0, indicating the distance between lines should be the standard
  // one for the label's text, font list, and platform.
  int line_height() const { return full_text_->min_line_height(); }
  void SetLineHeight(int height);

  // Get or set if the label text can wrap on multiple lines; default is false.
  bool multi_line() const { return multi_line_; }
  void SetMultiLine(bool multi_line);

  // If multi-line, a non-zero value will cap the number of lines rendered, and
  // elide the rest (currently only ELIDE_TAIL supported). See gfx::RenderText.
  int max_lines() const { return max_lines_; }
  void SetMaxLines(int max_lines);

  // Get or set if the label text should be obscured before rendering (e.g.
  // should "Password!" display as "*********"); default is false.
  bool obscured() const { return full_text_->obscured(); }
  void SetObscured(bool obscured);

  // Sets whether multi-line text can wrap mid-word; the default is false.
  // TODO(mukai): allow specifying WordWrapBehavior.
  void SetAllowCharacterBreak(bool allow_character_break);

  // Sets the eliding or fading behavior, applied as necessary. The default is
  // to elide at the end. Eliding is not well-supported for multi-line labels.
  void SetElideBehavior(gfx::ElideBehavior elide_behavior);
  gfx::ElideBehavior elide_behavior() const { return elide_behavior_; }

  // Sets the tooltip text.  Default behavior for a label (single-line) is to
  // show the full text if it is wider than its bounds.  Calling this overrides
  // the default behavior and lets you set a custom tooltip.  To revert to
  // default behavior, call this with an empty string.
  void SetTooltipText(const base::string16& tooltip_text);

  // Get or set whether this label can act as a tooltip handler; the default is
  // true.  Set to false whenever an ancestor view should handle tooltips
  // instead.
  bool handles_tooltips() const { return handles_tooltips_; }
  void SetHandlesTooltips(bool enabled);

  // Resizes the label so its width is set to the fixed width and its height
  // deduced accordingly. Even if all widths of the lines are shorter than
  // |fixed_width|, the given value is applied to the element's width.
  // This is only intended for multi-line labels and is useful when the label's
  // text contains several lines separated with \n.
  // |fixed_width| is the fixed width that will be used (longer lines will be
  // wrapped).  If 0, no fixed width is enforced.
  void SizeToFit(int fixed_width);

  // Like SizeToFit, but uses a smaller width if possible.
  void SetMaximumWidth(int max_width);

  // Sets whether the preferred size is empty when the label is not visible.
  void set_collapse_when_hidden(bool value) { collapse_when_hidden_ = value; }

  // Get the text as displayed to the user, respecting the obscured flag.
  base::string16 GetDisplayTextForTesting();

  // Returns true if the label can be made selectable. For example, links do not
  // support text selection.
  // Subclasses should override this function in case they want to selectively
  // support text selection. If a subclass stops supporting text selection, it
  // should call SetSelectable(false).
  virtual bool IsSelectionSupported() const;

  // Returns true if the label is selectable. Default is false.
  bool selectable() const { return !!selection_controller_; }

  // Sets whether the label is selectable. False is returned if the call fails,
  // i.e. when selection is not supported but |selectable| is true. For example,
  // obscured labels do not support text selection.
  bool SetSelectable(bool selectable);

  // Returns true if the label has a selection.
  bool HasSelection() const;

  // Selects the entire text. NO-OP if the label is not selectable.
  void SelectAll();

  // Clears any active selection.
  void ClearSelection();

  // Selects the given text range. NO-OP if the label is not selectable or the
  // |range| endpoints don't lie on grapheme boundaries.
  void SelectRange(const gfx::Range& range);

  // View:
  int GetBaseline() const override;
  gfx::Size CalculatePreferredSize() const override;
  gfx::Size GetMinimumSize() const override;
  int GetHeightForWidth(int w) const override;
  void Layout() override;
  const char* GetClassName() const override;
  View* GetTooltipHandlerForPoint(const gfx::Point& point) override;
  bool CanProcessEventsWithinSubtree() const override;
  WordLookupClient* GetWordLookupClient() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool GetTooltipText(const gfx::Point& p,
                      base::string16* tooltip) const override;

 protected:
  // Create a single RenderText instance to actually be painted.
  virtual std::unique_ptr<gfx::RenderText> CreateRenderText() const;

  // Draw a focus ring. The default implementation does nothing.
  virtual void PaintFocusRing(gfx::Canvas* canvas) const;
  gfx::Rect GetFocusRingBounds() const;

  void PaintText(gfx::Canvas* canvas);

  // View:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void VisibilityChanged(View* starting_from, bool is_visible) override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;
  gfx::NativeCursor GetCursor(const ui::MouseEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseCaptureLost() override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool CanHandleAccelerators() const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(LabelTest, ResetRenderTextData);
  FRIEND_TEST_ALL_PREFIXES(LabelTest, MultilineSupportedRenderText);
  FRIEND_TEST_ALL_PREFIXES(LabelTest, TextChangeWithoutLayout);
  FRIEND_TEST_ALL_PREFIXES(LabelTest, EmptyLabel);
  FRIEND_TEST_ALL_PREFIXES(MDLabelTest, FocusBounds);
  FRIEND_TEST_ALL_PREFIXES(LabelTest, MultiLineSizingWithElide);
  friend class LabelSelectionTest;

  // ContextMenuController overrides:
  void ShowContextMenuForView(View* source,
                              const gfx::Point& point,
                              ui::MenuSourceType source_type) override;

  // WordLookupClient overrides:
  bool GetWordLookupDataAtPoint(const gfx::Point& point,
                                gfx::DecoratedText* decorated_word,
                                gfx::Point* baseline_point) override;

  bool GetWordLookupDataFromSelection(gfx::DecoratedText* decorated_text,
                                      gfx::Point* baseline_point) override;

  // SelectionControllerDelegate overrides:
  gfx::RenderText* GetRenderTextForSelectionController() override;
  bool IsReadOnly() const override;
  bool SupportsDrag() const override;
  bool HasTextBeingDragged() const override;
  void SetTextBeingDragged(bool value) override;
  int GetViewHeight() const override;
  int GetViewWidth() const override;
  int GetDragSelectionDelay() const override;
  void OnBeforePointerAction() override;
  void OnAfterPointerAction(bool text_changed, bool selection_changed) override;
  bool PasteSelectionClipboard() override;
  void UpdateSelectionClipboard() override;

  // ui::SimpleMenuModel::Delegate overrides:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;
  bool GetAcceleratorForCommandId(int command_id,
                                  ui::Accelerator* accelerator) const override;

  const gfx::RenderText* GetRenderTextForSelectionController() const;

  void Init(const base::string16& text, const gfx::FontList& font_list);

  void ResetLayout();

  // Set up |display_text_| to actually be painted.
  void MaybeBuildDisplayText() const;

  // Get the text size for the current layout.
  gfx::Size GetTextSize() const;

  // Updates text and selection colors from requested colors.
  void RecalculateColors();

  // Applies the foreground color to |display_text_|.
  void ApplyTextColors() const;

  // Updates any colors that have not been explicitly set from the theme.
  void UpdateColorsFromTheme(const ui::NativeTheme* theme);

  bool ShouldShowDefaultTooltip() const;

  // Clears |display_text_| and updates |stored_selection_range_|.
  void ClearDisplayText() const;

  // Returns the currently selected text.
  base::string16 GetSelectedText() const;

  // Updates the clipboard with the currently selected text.
  void CopyToClipboard();

  // Builds |context_menu_contents_|.
  void BuildContextMenuContents();

  const int text_context_;

  // An un-elided and single-line RenderText object used for preferred sizing.
  std::unique_ptr<gfx::RenderText> full_text_;

  // The RenderText instance used for drawing.
  mutable std::unique_ptr<gfx::RenderText> display_text_;

  // Persists the current selection range between the calls to
  // ClearDisplayText() and MaybeBuildDisplayText(). Holds an InvalidRange when
  // not in use.
  mutable gfx::Range stored_selection_range_;

  SkColor requested_enabled_color_ = SK_ColorRED;
  SkColor actual_enabled_color_ = SK_ColorRED;
  SkColor background_color_ = SK_ColorRED;
  SkColor requested_selection_text_color_ = SK_ColorRED;
  SkColor actual_selection_text_color_ = SK_ColorRED;
  SkColor selection_background_color_ = SK_ColorRED;

  // Set to true once the corresponding setter is invoked.
  bool enabled_color_set_;
  bool background_color_set_;
  bool selection_text_color_set_;
  bool selection_background_color_set_;

  gfx::ElideBehavior elide_behavior_;

  bool subpixel_rendering_enabled_;
  bool auto_color_readability_;
  // TODO(mukai): remove |multi_line_| when all RenderText can render multiline.
  bool multi_line_;
  int max_lines_;
  base::string16 tooltip_text_;
  bool handles_tooltips_;
  // Whether to collapse the label when it's not visible.
  bool collapse_when_hidden_;
  int fixed_width_;
  int max_width_;

  std::unique_ptr<SelectionController> selection_controller_;

  // Context menu related members.
  ui::SimpleMenuModel context_menu_contents_;
  std::unique_ptr<views::MenuRunner> context_menu_runner_;

  DISALLOW_COPY_AND_ASSIGN(Label);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_LABEL_H_
