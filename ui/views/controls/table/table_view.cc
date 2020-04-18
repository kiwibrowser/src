// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/table/table_view.h"

#include <stddef.h>

#include <algorithm>
#include <map>
#include <utility>

#include "base/auto_reset.h"
#include "base/i18n/rtl.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/paint/paint_flags.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/text_utils.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/table/table_grouper.h"
#include "ui/views/controls/table/table_header.h"
#include "ui/views/controls/table/table_utils.h"
#include "ui/views/controls/table/table_view_observer.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/style/typography.h"

namespace views {

namespace {

constexpr int kGroupingIndicatorSize = 6;

// Returns result, unless ascending is false in which case -result is returned.
int SwapCompareResult(int result, bool ascending) {
  return ascending ? result : -result;
}

// Populates |model_index_to_range_start| based on the |grouper|.
void GetModelIndexToRangeStart(TableGrouper* grouper,
                               int row_count,
                               std::map<int, int>* model_index_to_range_start) {
  for (int model_index = 0; model_index < row_count;) {
    GroupRange range;
    grouper->GetGroupRange(model_index, &range);
    DCHECK_GT(range.length, 0);
    for (int range_counter = 0; range_counter < range.length; range_counter++)
      (*model_index_to_range_start)[range_counter + model_index] = model_index;
    model_index += range.length;
  }
}

// Returns the color id for the background of selected text. |has_focus|
// indicates if the table has focus.
ui::NativeTheme::ColorId text_background_color_id(bool has_focus) {
  return has_focus ?
      ui::NativeTheme::kColorId_TableSelectionBackgroundFocused :
      ui::NativeTheme::kColorId_TableSelectionBackgroundUnfocused;
}

// Returns the color id for text. |has_focus| indicates if the table has focus.
ui::NativeTheme::ColorId selected_text_color_id(bool has_focus) {
  return has_focus ? ui::NativeTheme::kColorId_TableSelectedText :
      ui::NativeTheme::kColorId_TableSelectedTextUnfocused;
}

// Whether the platform "command" key is down.
bool IsCmdOrCtrl(const ui::Event& event) {
#if defined(OS_MACOSX)
  return event.IsCommandDown();
#else
  return event.IsControlDown();
#endif
}

} // namespace

// Used as the comparator to sort the contents of the table.
struct TableView::SortHelper {
  explicit SortHelper(TableView* table) : table(table) {}

  bool operator()(int model_index1, int model_index2) {
    return table->CompareRows(model_index1, model_index2) < 0;
  }

  TableView* table;
};

// Used as the comparator to sort the contents of the table when a TableGrouper
// is present. When groups are present we sort the groups based on the first row
// in the group and within the groups we keep the same order as the model.
struct TableView::GroupSortHelper {
  explicit GroupSortHelper(TableView* table) : table(table) {}

  bool operator()(int model_index1, int model_index2) {
    const int range1 = model_index_to_range_start[model_index1];
    const int range2 = model_index_to_range_start[model_index2];
    if (range1 == range2) {
      // The two rows are in the same group, sort so that items in the same
      // group always appear in the same order.
      return model_index1 < model_index2;
    }
    return table->CompareRows(range1, range2) < 0;
  }

  TableView* table;
  std::map<int, int> model_index_to_range_start;
};

TableView::VisibleColumn::VisibleColumn() : x(0), width(0) {}

TableView::VisibleColumn::~VisibleColumn() {}

TableView::PaintRegion::PaintRegion()
    : min_row(0),
      max_row(0),
      min_column(0),
      max_column(0) {
}

TableView::PaintRegion::~PaintRegion() {}

// static
const char TableView::kViewClassName[] = "TableView";

TableView::TableView(ui::TableModel* model,
                     const std::vector<ui::TableColumn>& columns,
                     TableTypes table_type,
                     bool single_selection)
    : model_(NULL),
      columns_(columns),
      header_(NULL),
      table_type_(table_type),
      single_selection_(single_selection),
      select_on_remove_(true),
      observer_(NULL),
      last_parent_width_(0),
      layout_width_(0),
      grouper_(NULL),
      in_set_visible_column_width_(false) {
  constexpr int kTextContext = style::CONTEXT_TABLE_ROW;
  constexpr int kTextStyle = style::STYLE_PRIMARY;
  font_list_ = style::GetFont(kTextContext, kTextStyle);
  row_height_ = LayoutProvider::GetControlHeightForFont(kTextContext,
                                                        kTextStyle, font_list_);

  for (size_t i = 0; i < columns.size(); ++i) {
    VisibleColumn visible_column;
    visible_column.column = columns[i];
    visible_columns_.push_back(visible_column);
  }
  // Always focusable, even on Mac (consistent with NSTableView).
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetModel(model);
}

TableView::~TableView() {
  if (model_)
    model_->SetObserver(NULL);
}

// TODO: this doesn't support arbitrarily changing the model, rename this to
// ClearModel() or something.
void TableView::SetModel(ui::TableModel* model) {
  if (model == model_)
    return;

  if (model_)
    model_->SetObserver(NULL);
  model_ = model;
  selection_model_.Clear();
  if (model_)
    model_->SetObserver(this);
}

View* TableView::CreateParentIfNecessary() {
  ScrollView* scroll_view = ScrollView::CreateScrollViewWithBorder();
  scroll_view->SetContents(this);
  CreateHeaderIfNecessary();
  if (header_)
    scroll_view->SetHeader(header_);
  return scroll_view;
}

void TableView::SetGrouper(TableGrouper* grouper) {
  grouper_ = grouper;
  SortItemsAndUpdateMapping();
}

int TableView::RowCount() const {
  return model_ ? model_->RowCount() : 0;
}

void TableView::Select(int model_row) {
  if (!model_)
    return;

  SelectByViewIndex(model_row == -1 ? -1 : ModelToView(model_row));
}

int TableView::FirstSelectedRow() {
  return selection_model_.empty() ? -1 : selection_model_.selected_indices()[0];
}

void TableView::SetColumnVisibility(int id, bool is_visible) {
  if (is_visible == IsColumnVisible(id))
    return;

  if (is_visible) {
    VisibleColumn visible_column;
    visible_column.column = FindColumnByID(id);
    visible_columns_.push_back(visible_column);
  } else {
    for (size_t i = 0; i < visible_columns_.size(); ++i) {
      if (visible_columns_[i].column.id == id) {
        visible_columns_.erase(visible_columns_.begin() + i);
        break;
      }
    }
  }
  UpdateVisibleColumnSizes();
  PreferredSizeChanged();
  SchedulePaint();
  if (header_)
    header_->SchedulePaint();
}

void TableView::ToggleSortOrder(int visible_column_index) {
  DCHECK(visible_column_index >= 0 &&
         visible_column_index < static_cast<int>(visible_columns_.size()));
  const ui::TableColumn& column = visible_columns_[visible_column_index].column;
  if (!column.sortable)
    return;
  SortDescriptors sort(sort_descriptors_);
  if (!sort.empty() && sort[0].column_id == column.id) {
    if (sort[0].ascending == column.initial_sort_is_ascending) {
      // First toggle inverts the order.
      sort[0].ascending = !sort[0].ascending;
    } else {
      // Second toggle clears the sort.
      sort.clear();
    }
  } else {
    SortDescriptor descriptor(column.id, column.initial_sort_is_ascending);
    sort.insert(sort.begin(), descriptor);
    // Only persist two sort descriptors.
    if (sort.size() > 2)
      sort.resize(2);
  }
  SetSortDescriptors(sort);
}

void TableView::SetSortDescriptors(const SortDescriptors& sort_descriptors) {
  sort_descriptors_ = sort_descriptors;
  SortItemsAndUpdateMapping();
  if (header_)
    header_->SchedulePaint();
}

bool TableView::IsColumnVisible(int id) const {
  for (size_t i = 0; i < visible_columns_.size(); ++i) {
    if (visible_columns_[i].column.id == id)
      return true;
  }
  return false;
}

void TableView::AddColumn(const ui::TableColumn& col) {
  DCHECK(!HasColumn(col.id));
  columns_.push_back(col);
}

bool TableView::HasColumn(int id) const {
  for (size_t i = 0; i < columns_.size(); ++i) {
    if (columns_[i].id == id)
      return true;
  }
  return false;
}

const TableView::VisibleColumn& TableView::GetVisibleColumn(int index) {
  DCHECK(index >= 0 && index < static_cast<int>(visible_columns_.size()));
  return visible_columns_[index];
}

void TableView::SetVisibleColumnWidth(int index, int width) {
  DCHECK(index >= 0 && index < static_cast<int>(visible_columns_.size()));
  if (visible_columns_[index].width == width)
    return;
  base::AutoReset<bool> reseter(&in_set_visible_column_width_, true);
  visible_columns_[index].width = width;
  for (size_t i = index + 1; i < visible_columns_.size(); ++i) {
    visible_columns_[i].x =
        visible_columns_[i - 1].x + visible_columns_[i - 1].width;
  }
  PreferredSizeChanged();
  SchedulePaint();
}

int TableView::ModelToView(int model_index) const {
  if (!is_sorted())
    return model_index;
  DCHECK_GE(model_index, 0) << " negative model_index " << model_index;
  DCHECK_LT(model_index, RowCount()) << " out of bounds model_index " <<
      model_index;
  return model_to_view_[model_index];
}

int TableView::ViewToModel(int view_index) const {
  DCHECK_GE(view_index, 0) << " negative view_index " << view_index;
  DCHECK_LT(view_index, RowCount()) << " out of bounds view_index " <<
      view_index;
  return is_sorted() ? view_to_model_[view_index] : view_index;
}

void TableView::Layout() {
  // parent()->parent() is the scrollview. When its width changes we force
  // recalculating column sizes.
  View* scroll_view = parent() ? parent()->parent() : NULL;
  if (scroll_view) {
    const int scroll_view_width = scroll_view->GetContentsBounds().width();
    if (scroll_view_width != last_parent_width_) {
      last_parent_width_ = scroll_view_width;
      if (!in_set_visible_column_width_) {
        // Layout to the parent (the Viewport), which differs from
        // |scroll_view_width| when scrollbars are present.
        layout_width_ = parent()->width();
        UpdateVisibleColumnSizes();
      }
    }
  }
  // We have to override Layout like this since we're contained in a ScrollView.
  gfx::Size pref = GetPreferredSize();
  int width = pref.width();
  int height = pref.height();
  if (parent()) {
    width = std::max(parent()->width(), width);
    height = std::max(parent()->height(), height);
  }
  SetBounds(x(), y(), width, height);
}

const char* TableView::GetClassName() const {
  return kViewClassName;
}

gfx::Size TableView::CalculatePreferredSize() const {
  int width = 50;
  if (header_ && !visible_columns_.empty())
    width = visible_columns_.back().x + visible_columns_.back().width;
  return gfx::Size(width, RowCount() * row_height_);
}

bool TableView::OnKeyPressed(const ui::KeyEvent& event) {
  if (!HasFocus())
    return false;

  switch (event.key_code()) {
    case ui::VKEY_A:
      // control-a selects all.
      if (IsCmdOrCtrl(event) && !single_selection_ && RowCount()) {
        ui::ListSelectionModel selection_model;
        selection_model.SetSelectedIndex(selection_model_.active());
        for (int i = 0; i < RowCount(); ++i)
          selection_model.AddIndexToSelection(i);
        SetSelectionModel(std::move(selection_model));
        return true;
      }
      break;

    case ui::VKEY_HOME:
      if (RowCount())
        SelectByViewIndex(0);
      return true;

    case ui::VKEY_END:
      if (RowCount())
        SelectByViewIndex(RowCount() - 1);
      return true;

    case ui::VKEY_UP:
      AdvanceSelection(ADVANCE_DECREMENT);
      return true;

    case ui::VKEY_DOWN:
      AdvanceSelection(ADVANCE_INCREMENT);
      return true;

    default:
      break;
  }
  if (observer_)
    observer_->OnKeyDown(event.key_code());
  return false;
}

bool TableView::OnMousePressed(const ui::MouseEvent& event) {
  RequestFocus();
  if (!event.IsOnlyLeftMouseButton())
    return true;

  const int row = event.y() / row_height_;
  if (row < 0 || row >= RowCount())
    return true;

  if (event.GetClickCount() == 2) {
    SelectByViewIndex(row);
    if (observer_)
      observer_->OnDoubleClick();
  } else if (event.GetClickCount() == 1) {
    ui::ListSelectionModel new_model;
    ConfigureSelectionModelForEvent(event, &new_model);
    SetSelectionModel(std::move(new_model));
  }

  return true;
}

void TableView::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() != ui::ET_GESTURE_TAP_DOWN)
    return;

  RequestFocus();

  const int row = event->y() / row_height_;
  if (row < 0 || row >= RowCount())
    return;

  event->StopPropagation();
  ui::ListSelectionModel new_model;
  ConfigureSelectionModelForEvent(*event, &new_model);
  SetSelectionModel(std::move(new_model));
}

bool TableView::GetTooltipText(const gfx::Point& p,
                               base::string16* tooltip) const {
  return GetTooltipImpl(p, tooltip, NULL);
}

bool TableView::GetTooltipTextOrigin(const gfx::Point& p,
                                     gfx::Point* loc) const {
  return GetTooltipImpl(p, NULL, loc);
}

void TableView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  // TODO(aleventhal) Needs work, see https://crbug.com/811277.
  node_data->role = ax::mojom::Role::kTable;
  node_data->SetRestriction(ax::mojom::Restriction::kReadOnly);
  node_data->AddIntAttribute(ax::mojom::IntAttribute::kSetSize, RowCount());

  if (selection_model_.active() != ui::ListSelectionModel::kUnselectedIndex) {
    // Get information about the active item, this is not the same as the set
    // of selected items (of which there could be more than one).
    node_data->role = ax::mojom::Role::kRow;
    node_data->AddIntAttribute(ax::mojom::IntAttribute::kPosInSet,
                               selection_model_.active());
    node_data->AddBoolAttribute(
        ax::mojom::BoolAttribute::kSelected,
        selection_model_.IsSelected(selection_model_.active()));

    // Generate accessible name from column headers and selected cell text.
    std::vector<base::string16> name_parts;
    for (const VisibleColumn& visible_column : visible_columns_) {
      base::string16 value = model_->GetText(
          selection_model_.active(), visible_column.column.id);
      if (!value.empty()) {
        name_parts.push_back(visible_column.column.title);
        name_parts.push_back(value);
      }
    }
    node_data->SetName(base::JoinString(name_parts, base::ASCIIToUTF16(", ")));
  } else {
    // Name requires a selection.
    node_data->SetNameExplicitlyEmpty();
  }
}

void TableView::OnModelChanged() {
  selection_model_.Clear();
  NumRowsChanged();
}

void TableView::OnItemsChanged(int start, int length) {
  SortItemsAndUpdateMapping();
}

void TableView::OnItemsAdded(int start, int length) {
  for (int i = 0; i < length; ++i)
    selection_model_.IncrementFrom(start);
  NumRowsChanged();
}

void TableView::OnItemsMoved(int old_start, int length, int new_start) {
  selection_model_.Move(old_start, new_start, length);
  SortItemsAndUpdateMapping();
}

void TableView::OnItemsRemoved(int start, int length) {
  DCHECK_GE(start, 0);
  // Determine the currently selected index in terms of the view. We inline the
  // implementation here since ViewToModel() has DCHECKs that fail since the
  // model has changed but |model_to_view_| has not been updated yet.
  const int previously_selected_model_index = FirstSelectedRow();
  int previously_selected_view_index = previously_selected_model_index;
  if (previously_selected_model_index != -1 && is_sorted())
    previously_selected_view_index =
        model_to_view_[previously_selected_model_index];
  for (int i = 0; i < length; ++i)
    selection_model_.DecrementFrom(start);
  NumRowsChanged();
  // If the selection was empty and is no longer empty select the same visual
  // index.
  if (selection_model_.empty() && previously_selected_view_index != -1 &&
      RowCount() && select_on_remove_) {
    selection_model_.SetSelectedIndex(
        ViewToModel(std::min(RowCount() - 1, previously_selected_view_index)));
  }
  if (!selection_model_.empty() && selection_model_.active() == -1)
    selection_model_.set_active(FirstSelectedRow());
  if (!selection_model_.empty() && selection_model_.anchor() == -1)
    selection_model_.set_anchor(FirstSelectedRow());
  if (observer_)
    observer_->OnSelectionChanged();
}

gfx::Point TableView::GetKeyboardContextMenuLocation() {
  int first_selected = FirstSelectedRow();
  gfx::Rect vis_bounds(GetVisibleBounds());
  int y = vis_bounds.height() / 2;
  if (first_selected != -1) {
    gfx::Rect cell_bounds(GetRowBounds(first_selected));
    if (cell_bounds.bottom() >= vis_bounds.y() &&
        cell_bounds.bottom() < vis_bounds.bottom()) {
      y = cell_bounds.bottom();
    }
  }
  gfx::Point screen_loc(0, y);
  if (base::i18n::IsRTL())
    screen_loc.set_x(width());
  ConvertPointToScreen(this, &screen_loc);
  return screen_loc;
}

void TableView::OnPaint(gfx::Canvas* canvas) {
  // Don't invoke View::OnPaint so that we can render our own focus border.

  canvas->DrawColor(GetNativeTheme()->GetSystemColor(
                        ui::NativeTheme::kColorId_TableBackground));

  if (!RowCount() || visible_columns_.empty())
    return;

  const PaintRegion region(GetPaintRegion(GetPaintBounds(canvas)));
  if (region.min_column == -1)
    return;  // No need to paint anything.

  const SkColor selected_bg_color = GetNativeTheme()->GetSystemColor(
      text_background_color_id(HasFocus()));
  const SkColor fg_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_TableText);
  const SkColor selected_fg_color = GetNativeTheme()->GetSystemColor(
      selected_text_color_id(HasFocus()));
  const int cell_margin = GetCellMargin();
  const int cell_element_spacing = GetCellElementSpacing();
  for (int i = region.min_row; i < region.max_row; ++i) {
    const int model_index = ViewToModel(i);
    const bool is_selected = selection_model_.IsSelected(model_index);
    if (is_selected)
      canvas->FillRect(GetRowBounds(i), selected_bg_color);
    for (int j = region.min_column; j < region.max_column; ++j) {
      const gfx::Rect cell_bounds(GetCellBounds(i, j));
      int text_x = cell_margin + cell_bounds.x();

      // Provide space for the grouping indicator, but draw it separately.
      if (j == 0 && grouper_)
        text_x += kGroupingIndicatorSize + cell_element_spacing;

      // Always paint the icon in the first visible column.
      if (j == 0 && table_type_ == ICON_AND_TEXT) {
        gfx::ImageSkia image = model_->GetIcon(model_index);
        if (!image.isNull()) {
          int image_x =
              GetMirroredXWithWidthInView(text_x, ui::TableModel::kIconSize);
          canvas->DrawImageInt(
              image, 0, 0, image.width(), image.height(), image_x,
              cell_bounds.y() +
                  (cell_bounds.height() - ui::TableModel::kIconSize) / 2,
              ui::TableModel::kIconSize, ui::TableModel::kIconSize, true);
        }
        text_x += ui::TableModel::kIconSize + cell_element_spacing;
      }
      if (text_x < cell_bounds.right() - cell_margin) {
        canvas->DrawStringRectWithFlags(
            model_->GetText(model_index, visible_columns_[j].column.id),
            font_list_, is_selected ? selected_fg_color : fg_color,
            gfx::Rect(GetMirroredXWithWidthInView(
                          text_x, cell_bounds.right() - text_x - cell_margin),
                      cell_bounds.y(), cell_bounds.right() - text_x,
                      row_height_),
            TableColumnAlignmentToCanvasAlignment(
                visible_columns_[j].column.alignment));
      }
    }
  }

  if (!grouper_ || region.min_column > 0)
    return;

  const SkColor grouping_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_TableGroupingIndicatorColor);
  cc::PaintFlags grouping_flags;
  grouping_flags.setColor(grouping_color);
  grouping_flags.setStyle(cc::PaintFlags::kFill_Style);
  grouping_flags.setAntiAlias(true);
  const int group_indicator_x = GetMirroredXInView(
      GetCellBounds(0, 0).x() + cell_margin + kGroupingIndicatorSize / 2);
  for (int i = region.min_row; i < region.max_row; ) {
    const int model_index = ViewToModel(i);
    GroupRange range;
    grouper_->GetGroupRange(model_index, &range);
    DCHECK_GT(range.length, 0);
    // The order of rows in a group is consistent regardless of sort, so it's ok
    // to do this calculation.
    const int start = i - (model_index - range.start);
    const int last = start + range.length - 1;
    const gfx::Rect start_cell_bounds(GetCellBounds(start, 0));
    if (start != last) {
      const gfx::Rect last_cell_bounds(GetCellBounds(last, 0));
      canvas->FillRect(gfx::Rect(
                           group_indicator_x - kGroupingIndicatorSize / 2,
                           start_cell_bounds.CenterPoint().y(),
                           kGroupingIndicatorSize,
                           last_cell_bounds.y() - start_cell_bounds.y()),
                       grouping_color);
      canvas->DrawCircle(
          gfx::Point(group_indicator_x, last_cell_bounds.CenterPoint().y()),
          kGroupingIndicatorSize / 2, grouping_flags);
    }
    canvas->DrawCircle(
        gfx::Point(group_indicator_x, start_cell_bounds.CenterPoint().y()),
        kGroupingIndicatorSize / 2, grouping_flags);
    i = last + 1;
  }
}

void TableView::OnFocus() {
  ScrollView* scroll_view = ScrollView::GetScrollViewForContents(this);
  if (scroll_view)
    scroll_view->SetHasFocusIndicator(true);

  SchedulePaintForSelection();
  NotifyAccessibilityEvent(ax::mojom::Event::kFocus, true);
}

void TableView::OnBlur() {
  ScrollView* scroll_view = ScrollView::GetScrollViewForContents(this);
  if (scroll_view)
    scroll_view->SetHasFocusIndicator(false);
  SchedulePaintForSelection();
}

int TableView::GetCellMargin() const {
  return LayoutProvider::Get()->GetDistanceMetric(
      DISTANCE_TABLE_CELL_HORIZONTAL_MARGIN);
}

int TableView::GetCellElementSpacing() const {
  return LayoutProvider::Get()->GetDistanceMetric(
      DISTANCE_RELATED_LABEL_HORIZONTAL);
}

void TableView::NumRowsChanged() {
  SortItemsAndUpdateMapping();
  PreferredSizeChanged();
  SchedulePaint();
}

void TableView::SortItemsAndUpdateMapping() {
  if (!is_sorted()) {
    view_to_model_.clear();
    model_to_view_.clear();
  } else {
    const int row_count = RowCount();
    view_to_model_.resize(row_count);
    model_to_view_.resize(row_count);
    for (int i = 0; i < row_count; ++i)
      view_to_model_[i] = i;
    if (grouper_) {
      GroupSortHelper sort_helper(this);
      GetModelIndexToRangeStart(grouper_, RowCount(),
                                &sort_helper.model_index_to_range_start);
      std::stable_sort(view_to_model_.begin(), view_to_model_.end(),
                       sort_helper);
    } else {
      std::stable_sort(view_to_model_.begin(), view_to_model_.end(),
                       SortHelper(this));
    }
    for (int i = 0; i < row_count; ++i)
      model_to_view_[view_to_model_[i]] = i;
    model_->ClearCollator();
  }
  SchedulePaint();
}

int TableView::CompareRows(int model_row1, int model_row2) {
  const int sort_result = model_->CompareValues(
      model_row1, model_row2, sort_descriptors_[0].column_id);
  if (sort_result == 0 && sort_descriptors_.size() > 1) {
    // Try the secondary sort.
    return SwapCompareResult(
        model_->CompareValues(model_row1, model_row2,
                              sort_descriptors_[1].column_id),
        sort_descriptors_[1].ascending);
  }
  return SwapCompareResult(sort_result, sort_descriptors_[0].ascending);
}

gfx::Rect TableView::GetRowBounds(int row) const {
  return gfx::Rect(0, row * row_height_, width(), row_height_);
}

gfx::Rect TableView::GetCellBounds(int row, int visible_column_index) const {
  if (!header_)
    return GetRowBounds(row);
  const VisibleColumn& vis_col(visible_columns_[visible_column_index]);
  return gfx::Rect(vis_col.x, row * row_height_, vis_col.width, row_height_);
}

void TableView::AdjustCellBoundsForText(int visible_column_index,
                                        gfx::Rect* bounds) const {
  const int cell_margin = GetCellMargin();
  const int cell_element_spacing = GetCellElementSpacing();
  int text_x = cell_margin + bounds->x();
  if (visible_column_index == 0) {
    if (grouper_)
      text_x += kGroupingIndicatorSize + cell_element_spacing;
    if (table_type_ == ICON_AND_TEXT)
      text_x += ui::TableModel::kIconSize + cell_element_spacing;
  }
  bounds->set_x(text_x);
  bounds->set_width(std::max(0, bounds->right() - cell_margin - text_x));
}

void TableView::CreateHeaderIfNecessary() {
  // Only create a header if there is more than one column or the title of the
  // only column is not empty.
  if (header_ || (columns_.size() == 1 && columns_[0].title.empty()))
    return;

  header_ = new TableHeader(this);
}

void TableView::UpdateVisibleColumnSizes() {
  if (!header_)
    return;

  std::vector<ui::TableColumn> columns;
  for (size_t i = 0; i < visible_columns_.size(); ++i)
    columns.push_back(visible_columns_[i].column);

  const int cell_margin = GetCellMargin();
  const int cell_element_spacing = GetCellElementSpacing();
  int first_column_padding = 0;
  if (table_type_ == ICON_AND_TEXT && header_)
    first_column_padding += ui::TableModel::kIconSize + cell_element_spacing;
  if (grouper_)
    first_column_padding += kGroupingIndicatorSize + cell_element_spacing;

  std::vector<int> sizes = views::CalculateTableColumnSizes(
      layout_width_, first_column_padding, header_->font_list(), font_list_,
      std::max(cell_margin, TableHeader::kHorizontalPadding) * 2,
      TableHeader::kSortIndicatorWidth, columns, model_);
  DCHECK_EQ(visible_columns_.size(), sizes.size());
  int x = 0;
  for (size_t i = 0; i < visible_columns_.size(); ++i) {
    visible_columns_[i].x = x;
    visible_columns_[i].width = sizes[i];
    x += sizes[i];
  }
}

TableView::PaintRegion TableView::GetPaintRegion(
    const gfx::Rect& bounds) const {
  DCHECK(!visible_columns_.empty());
  DCHECK(RowCount());

  PaintRegion region;
  region.min_row = std::min(RowCount() - 1,
                            std::max(0, bounds.y() / row_height_));
  region.max_row = bounds.bottom() / row_height_;
  if (bounds.bottom() % row_height_ != 0)
    region.max_row++;
  region.max_row = std::min(region.max_row, RowCount());

  if (!header_) {
    region.max_column = 1;
    return region;
  }

  const int paint_x = GetMirroredXForRect(bounds);
  const int paint_max_x = paint_x + bounds.width();
  region.min_column = -1;
  region.max_column = visible_columns_.size();
  for (size_t i = 0; i < visible_columns_.size(); ++i) {
    int max_x = visible_columns_[i].x + visible_columns_[i].width;
    if (region.min_column == -1 && max_x >= paint_x)
      region.min_column = static_cast<int>(i);
    if (region.min_column != -1 && visible_columns_[i].x >= paint_max_x) {
      region.max_column = i;
      break;
    }
  }
  return region;
}

gfx::Rect TableView::GetPaintBounds(gfx::Canvas* canvas) const {
  SkRect sk_clip_rect;
  if (canvas->sk_canvas()->getLocalClipBounds(&sk_clip_rect))
    return gfx::ToEnclosingRect(gfx::SkRectToRectF(sk_clip_rect));
  return GetVisibleBounds();
}

void TableView::SchedulePaintForSelection() {
  if (selection_model_.size() == 1) {
    const int first_model_row = FirstSelectedRow();
    SchedulePaintInRect(GetRowBounds(ModelToView(first_model_row)));
    if (first_model_row != selection_model_.active())
      SchedulePaintInRect(GetRowBounds(ModelToView(selection_model_.active())));
  } else if (selection_model_.size() > 1) {
    SchedulePaint();
  }
}

ui::TableColumn TableView::FindColumnByID(int id) const {
  for (size_t i = 0; i < columns_.size(); ++i) {
    if (columns_[i].id == id)
      return columns_[i];
  }
  NOTREACHED();
  return ui::TableColumn();
}

void TableView::SelectByViewIndex(int view_index) {
  ui::ListSelectionModel new_selection;
  if (view_index != -1) {
    SelectRowsInRangeFrom(view_index, true, &new_selection);
    new_selection.set_anchor(ViewToModel(view_index));
    new_selection.set_active(ViewToModel(view_index));
  }

  SetSelectionModel(std::move(new_selection));
}

void TableView::SetSelectionModel(ui::ListSelectionModel new_selection) {
  if (new_selection == selection_model_)
    return;

  SchedulePaintForSelection();
  selection_model_ = std::move(new_selection);
  SchedulePaintForSelection();

  // Scroll the group for the active item to visible.
  if (selection_model_.active() != -1) {
    gfx::Rect vis_rect(GetVisibleBounds());
    const GroupRange range(GetGroupRange(selection_model_.active()));
    const int start_y = GetRowBounds(ModelToView(range.start)).y();
    const int end_y =
        GetRowBounds(ModelToView(range.start + range.length - 1)).bottom();
    vis_rect.set_y(start_y);
    vis_rect.set_height(end_y - start_y);
    ScrollRectToVisible(vis_rect);
  }

  if (observer_)
    observer_->OnSelectionChanged();

  NotifyAccessibilityEvent(ax::mojom::Event::kFocus, true);
}

void TableView::AdvanceSelection(AdvanceDirection direction) {
  if (selection_model_.active() == -1) {
    SelectByViewIndex(0);
    return;
  }
  int view_index = ModelToView(selection_model_.active());
  if (direction == ADVANCE_DECREMENT)
    view_index = std::max(0, view_index - 1);
  else
    view_index = std::min(RowCount() - 1, view_index + 1);
  SelectByViewIndex(view_index);
}

void TableView::ConfigureSelectionModelForEvent(
    const ui::LocatedEvent& event,
    ui::ListSelectionModel* model) const {
  const int view_index = event.y() / row_height_;
  DCHECK(view_index >= 0 && view_index < RowCount());

  if (selection_model_.anchor() == -1 || single_selection_ ||
      (!IsCmdOrCtrl(event) && !event.IsShiftDown())) {
    SelectRowsInRangeFrom(view_index, true, model);
    model->set_anchor(ViewToModel(view_index));
    model->set_active(ViewToModel(view_index));
    return;
  }
  if ((IsCmdOrCtrl(event) && event.IsShiftDown()) || event.IsShiftDown()) {
    // control-shift: copy existing model and make sure rows between anchor and
    // |view_index| are selected.
    // shift: reset selection so that only rows between anchor and |view_index|
    // are selected.
    if (IsCmdOrCtrl(event) && event.IsShiftDown())
      *model = selection_model_;
    else
      model->set_anchor(selection_model_.anchor());
    for (int i = std::min(view_index, ModelToView(model->anchor())),
             end = std::max(view_index, ModelToView(model->anchor()));
         i <= end; ++i) {
      SelectRowsInRangeFrom(i, true, model);
    }
    model->set_active(ViewToModel(view_index));
  } else {
    DCHECK(IsCmdOrCtrl(event));
    // Toggle the selection state of |view_index| and set the anchor/active to
    // it and don't change the state of any other rows.
    *model = selection_model_;
    model->set_anchor(ViewToModel(view_index));
    model->set_active(ViewToModel(view_index));
    SelectRowsInRangeFrom(view_index,
                          !model->IsSelected(ViewToModel(view_index)),
                          model);
  }
}

void TableView::SelectRowsInRangeFrom(int view_index,
                                      bool select,
                                      ui::ListSelectionModel* model) const {
  const GroupRange range(GetGroupRange(ViewToModel(view_index)));
  for (int i = 0; i < range.length; ++i) {
    if (select)
      model->AddIndexToSelection(range.start + i);
    else
      model->RemoveIndexFromSelection(range.start + i);
  }
}

GroupRange TableView::GetGroupRange(int model_index) const {
  GroupRange range;
  if (grouper_) {
    grouper_->GetGroupRange(model_index, &range);
  } else {
    range.start = model_index;
    range.length = 1;
  }
  return range;
}

bool TableView::GetTooltipImpl(const gfx::Point& location,
                               base::string16* tooltip,
                               gfx::Point* tooltip_origin) const {
  const int row = location.y() / row_height_;
  if (row < 0 || row >= RowCount() || visible_columns_.empty())
    return false;

  const int x = GetMirroredXInView(location.x());
  const int column = GetClosestVisibleColumnIndex(this, x);
  if (x < visible_columns_[column].x ||
      x > (visible_columns_[column].x + visible_columns_[column].width))
    return false;

  const base::string16 text(model_->GetText(ViewToModel(row),
                                      visible_columns_[column].column.id));
  if (text.empty())
    return false;

  gfx::Rect cell_bounds(GetCellBounds(row, column));
  AdjustCellBoundsForText(column, &cell_bounds);
  const int right = std::min(GetVisibleBounds().right(), cell_bounds.right());
  if (right > cell_bounds.x() &&
      gfx::GetStringWidth(text, font_list_) <= (right - cell_bounds.x()))
    return false;

  if (tooltip)
    *tooltip = text;
  if (tooltip_origin) {
    tooltip_origin->SetPoint(
        cell_bounds.x(),
        cell_bounds.y() + (row_height_ - font_list_.GetHeight()) / 2);
  }
  return true;
}

}  // namespace views
