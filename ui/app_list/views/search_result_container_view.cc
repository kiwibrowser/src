// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/search_result_container_view.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"

namespace app_list {

SearchResultContainerView::SearchResultContainerView()
    : delegate_(nullptr),
      selected_index_(-1),
      num_results_(0),
      results_(NULL),
      update_factory_(this) {
}

SearchResultContainerView::~SearchResultContainerView() {
  if (results_)
    results_->RemoveObserver(this);
}

void SearchResultContainerView::SetResults(
    SearchModel::SearchResults* results) {
  if (results_)
    results_->RemoveObserver(this);

  results_ = results;
  if (results_)
    results_->AddObserver(this);

  Update();
}

void SearchResultContainerView::SetSelectedIndex(int selected_index) {
  DCHECK(IsValidSelectionIndex(selected_index));
  int old_selected = selected_index_;
  selected_index_ = selected_index;
  UpdateSelectedIndex(old_selected, selected_index_);
}

void SearchResultContainerView::ClearSelectedIndex() {
  int old_selected = selected_index_;
  selected_index_ = -1;
  UpdateSelectedIndex(old_selected, selected_index_);
}

bool SearchResultContainerView::IsValidSelectionIndex(int index) const {
  return index >= 0 && index <= num_results() - 1;
}

void SearchResultContainerView::Update() {
  update_factory_.InvalidateWeakPtrs();
  num_results_ = DoUpdate();
  Layout();
  if (delegate_)
    delegate_->OnSearchResultContainerResultsChanged();
}

bool SearchResultContainerView::UpdateScheduled() {
  return update_factory_.HasWeakPtrs();
}

const char* SearchResultContainerView::GetClassName() const {
  return "SearchResultContainerView";
}

void SearchResultContainerView::ListItemsAdded(size_t start, size_t count) {
  ScheduleUpdate();
}

void SearchResultContainerView::ListItemsRemoved(size_t start, size_t count) {
  ScheduleUpdate();
}

void SearchResultContainerView::ListItemMoved(size_t index,
                                              size_t target_index) {
  ScheduleUpdate();
}

void SearchResultContainerView::ListItemsChanged(size_t start, size_t count) {
  ScheduleUpdate();
}

void SearchResultContainerView::ScheduleUpdate() {
  // When search results are added one by one, each addition generates an update
  // request. Consolidates those update requests into one Update call.
  if (!update_factory_.HasWeakPtrs()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&SearchResultContainerView::Update,
                                  update_factory_.GetWeakPtr()));
  }
}

}  // namespace app_list
