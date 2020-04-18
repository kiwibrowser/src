/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// ConditionVariableEvents provides a doubly-link-list of events for use
// exclusively by the ConditionVariable class.

// This custom container was crafted because no simple combination of STL
// classes appeared to support the functionality required.  The specific
// unusual requirement for a linked-list-class is support for the Extract()
// method, which can remove an element from a list, potentially for insertion
// into a second list.  Most critically, the Extract() method is idempotent,
// turning the indicated element into an extracted singleton whether it was
// contained in a list or not.  This functionality allows one (or more) of
// threads to do the extraction.  The iterator that identifies this extractable
// element (in this case, a pointer to the list element) can be used after
// arbitrary manipulation of the (possibly) enclosing list container.  In
// general, STL containers do not provide iterators that can be used across
// modifications (insertions/extractions) of the enclosing containers, and
// certainly don't provide iterators that can be used if the identified
// element is *deleted* (removed) from the container.

// It is possible to use multiple redundant containers, such as an STL list,
// and an STL map, to achieve similar container semantics.  This container has
// only O(1) methods, while the corresponding (multiple) STL container approach
// would have more complex O(log(N)) methods (yeah... N isn't that large).
// Multiple containers also makes correctness more difficult to assert, as
// data is redundantly stored and maintained, which is generally evil.

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_CONDITION_VARIABLE_EVENTS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_CONDITION_VARIABLE_EVENTS_H_

#include "native_client/src/shared/platform/win/lock.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_check.h"

namespace NaCl {

class ConditionVariable;

// Define elements that are used in a circular linked list.
// The list container is an element with zero as handle_ value.
// The actual list elements will have a non-zero HANDLE as their handle_.
// All access to methods MUST be done under protection of a lock so that links
// can be validated (and not asynchronously changing) during the method calls.
class ConditionVariableEvent {
 private:
  friend class ConditionVariable;

  HANDLE handle_;
  ConditionVariableEvent* next_;  // Doubly linked list.
  ConditionVariableEvent* prev_;

  // Default constructor with no arguments creates a list container.
  explicit ConditionVariableEvent(bool is_list_element = false) {
    if (is_list_element) {
      handle_ = CreateEvent(NULL, false, false, NULL);
      // DCHECK(0 != handle_);  // InitCheck will validate in production.
    } else {
      handle_ = 0;
    }
    next_ = prev_ = this;  // Self referencing circular.
  }

  ~ConditionVariableEvent() {
    // DCHECK(IsSingleton());
    if (0 != handle_) {
      if (0 == CloseHandle(handle_)) {
        NaClLog(LOG_ERROR, "CloseHandle returned 0"
                "in ~ConditionVariableEvent()");
      }
    }
  }

  // Methods for use on lists.
  bool IsEmpty() {
    // DCHECK(ValidateAsList());
    return IsSingleton();
  }

  void PushBack(ConditionVariableEvent* other) {
    // DCHECK(ValidateAsList());
    // DCHECK(other->ValidateAsItem());
    // DCHECK(other->IsSingleton());
    // Prepare other for insertion.
    other->prev_ = prev_;
    other->next_ = this;
    // Cut into list.
    prev_->next_ = other;
    prev_ = other;
    // DCHECK(ValidateAsDistinct(other));
  }

  ConditionVariableEvent* PopFront() {
    // DCHECK(ValidateAsList());
    // DCHECK(!IsSingleton());
    return next_->Extract();
  }

  ConditionVariableEvent* PopBack() {
    // DCHECK(ValidateAsList());
    // DCHECK(!IsSingleton());
    return prev_->Extract();
  }

  // Methods for use on list elements.
  // Accessor method.
  HANDLE handle() {
    // DCHECK(ValidateAsItem());
    return handle_;
  }

  // Pull an element from a list (if it's in one).
  ConditionVariableEvent* Extract() {
    // DCHECK(ValidateAsItem());
    if (!IsSingleton()) {
      // Stitch neighbors together.
      next_->prev_ = prev_;
      prev_->next_ = next_;
      // Make extractee into a singleton.
      prev_ = next_ = this;
    }
    // DCHECK(IsSingleton());
    return this;
  }

  // Method for use on a list element or on a list.
  bool IsSingleton() {
    // DCHECK(ValidateLinks());
    return next_ == this;
  }

  // Provide pre/post conditions to validate correct manipulations.
  bool ValidateAsDistinct(ConditionVariableEvent* other) {
    return ValidateLinks() && other->ValidateLinks() && (this != other);
  }

  bool ValidateAsItem() {
    return (0 != handle_) && ValidateLinks();
  }

  bool ValidateAsList() {
    return (0 == handle_) && ValidateLinks();
  }

  bool ValidateLinks() {
    // Make sure our neighbor's link to us.
    return (next_->prev_ == this) && (prev_->next_ == this);
  }

  NACL_DISALLOW_COPY_AND_ASSIGN(ConditionVariableEvent);
};

}  // namespace NaCl

#endif  // NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_CONDITION_VARIABLE_EVENTS_H_
