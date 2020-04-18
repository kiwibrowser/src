// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web_state_list/web_state_list_order_controller.h"

#include <cstdint>

#include "base/logging.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

WebStateListOrderController::WebStateListOrderController(
    WebStateList* web_state_list)
    : web_state_list_(web_state_list) {
  DCHECK(web_state_list_);
}

WebStateListOrderController::~WebStateListOrderController() = default;

int WebStateListOrderController::DetermineInsertionIndex(
    web::WebState* opener) const {
  if (!opener)
    return web_state_list_->count();

  int opener_index = web_state_list_->GetIndexOfWebState(opener);
  DCHECK_NE(WebStateList::kInvalidIndex, opener_index);

  int list_child_index = web_state_list_->GetIndexOfLastWebStateOpenedBy(
      opener, opener_index, true);

  int reference_index = list_child_index != WebStateList::kInvalidIndex
                            ? list_child_index
                            : opener_index;

  // Check for overflows (just a DCHECK as INT_MAX open WebState is unlikely).
  DCHECK_LT(reference_index, INT_MAX);
  return reference_index + 1;
}

int WebStateListOrderController::DetermineNewActiveIndex(
    int removing_index) const {
  DCHECK(web_state_list_->ContainsIndex(removing_index));
  // First see if the index being removed has any "child" WebState. If it does,
  // select the first WebState in that child group, not the next in the removed
  // index group.
  int index = web_state_list_->GetIndexOfNextWebStateOpenedBy(
      web_state_list_->GetWebStateAt(removing_index), removing_index, false);

  if (index != WebStateList::kInvalidIndex)
    return GetValidIndex(index, removing_index);

  web::WebState* opener =
      web_state_list_->GetOpenerOfWebStateAt(removing_index).opener;
  if (opener) {
    // If the WebState was in a group, shift selection to the next WebState in
    // the group.
    int index = web_state_list_->GetIndexOfNextWebStateOpenedBy(
        opener, removing_index, false);

    if (index != WebStateList::kInvalidIndex)
      return GetValidIndex(index, removing_index);

    // If there is no subsequent group member, just fall back to opener itself.
    index = web_state_list_->GetIndexOfWebState(opener);
    return GetValidIndex(index, removing_index);
  }

  // If this is the last WebState in the WebStateList, clear the selection.
  if (web_state_list_->count() == 1)
    return WebStateList::kInvalidIndex;

  // No opener, fall through to the default handler, i.e. returning the previous
  // WebState if the removed one is the last, otherwise returning the next one.
  if (removing_index >= web_state_list_->count() - 1)
    return removing_index - 1;

  return removing_index;
}

int WebStateListOrderController::GetValidIndex(int index,
                                               int removing_index) const {
  if (removing_index < index)
    return std::min(0, index - 1);
  return index;
}
