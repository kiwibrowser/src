// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_SEARCH_RESULT_CONTAINER_VIEW_H_
#define UI_APP_LIST_VIEWS_SEARCH_RESULT_CONTAINER_VIEW_H_

#include <stddef.h>

#include "ash/app_list/model/app_list_model.h"
#include "ash/app_list/model/search/search_model.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/app_list/app_list_export.h"
#include "ui/views/view.h"

namespace app_list {

class SearchResultBaseView;

// SearchResultContainerView is a base class for views that contain multiple
// search results. SearchPageView holds these in a list and manages which one is
// selected. There can be one result within one SearchResultContainerView
// selected at a time; moving off the end of one container view selects the
// first element of the next container view, and vice versa
class APP_LIST_EXPORT SearchResultContainerView : public views::View,
                                                  public ui::ListModelObserver {
 public:
  class Delegate {
   public:
    virtual void OnSearchResultContainerResultsChanged() = 0;
  };
  SearchResultContainerView();
  ~SearchResultContainerView() override;

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // Sets the search results to listen to.
  void SetResults(SearchModel::SearchResults* results);
  SearchModel::SearchResults* results() { return results_; }

  // Sets the index of the selected search result within this container. This
  // must be a valid index.
  void SetSelectedIndex(int selected_index);

  void ClearSelectedIndex();

  // The currently selected index. Returns -1 on no selection.
  int selected_index() const { return selected_index_; }

  // Returns whether |index| is a valid index for selection.
  bool IsValidSelectionIndex(int index) const;

  int num_results() const { return num_results_; }

  void set_container_score(double score) { container_score_ = score; }
  double container_score() const { return container_score_; }

  // Updates the distance_from_origin() properties of the results in this
  // container. |y_index| is the absolute y-index of the first result of this
  // container (counting from the top of the app list).
  virtual void NotifyFirstResultYIndex(int y_index) = 0;

  // Gets the number of down keystrokes from the beginning to the end of this
  // container.
  virtual int GetYSize() = 0;

  // Batching method that actually performs the update and updates layout.
  void Update();

  // Returns whether an update is currently scheduled for this container.
  bool UpdateScheduled();

  // Overridden from views::View:
  const char* GetClassName() const override;

  // Overridden from ui::ListModelObserver:
  void ListItemsAdded(size_t start, size_t count) override;
  void ListItemsRemoved(size_t start, size_t count) override;
  void ListItemMoved(size_t index, size_t target_index) override;
  void ListItemsChanged(size_t start, size_t count) override;

  // Updates the container for being selected. |from_bottom| is true if the view
  // was entered into from a selected view below it; false if entered into from
  // above. |directional_movement| is true if the navigation was caused by
  // directional controls (eg, arrow keys), as opposed to linear controls (eg,
  // Tab).
  virtual void OnContainerSelected(bool from_bottom,
                                   bool directional_movement) = 0;

  // Returns selected view in this container view.
  virtual views::View* GetSelectedView() = 0;

  // Returns the first result in the container view. Returns NULL if it does not
  // exist.
  virtual SearchResultBaseView* GetFirstResultView() = 0;

 private:
  // Schedules an Update call using |update_factory_|. Do nothing if there is a
  // pending call.
  void ScheduleUpdate();

  // Updates UI with model. Returns the number of visible results.
  virtual int DoUpdate() = 0;

  // Updates UI for a change in the selected index.
  virtual void UpdateSelectedIndex(int old_selected, int new_selected) = 0;

  Delegate* delegate_;

  int selected_index_;
  int num_results_;

  double container_score_;

  SearchModel::SearchResults* results_;  // Owned by SearchModel.

  // The factory that consolidates multiple Update calls into one.
  base::WeakPtrFactory<SearchResultContainerView> update_factory_;

  DISALLOW_COPY_AND_ASSIGN(SearchResultContainerView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_SEARCH_RESULT_CONTAINER_VIEW_H_
