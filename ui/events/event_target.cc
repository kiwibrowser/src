// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/event_target.h"

#include <algorithm>

#include "base/logging.h"
#include "ui/events/event.h"

namespace ui {

EventTarget::EventTarget()
    : target_handler_(NULL) {
}

EventTarget::~EventTarget() {
}

void EventTarget::ConvertEventToTarget(EventTarget* target,
                                       LocatedEvent* event) {
}

void EventTarget::AddPreTargetHandler(EventHandler* handler,
                                      Priority priority) {
  DCHECK(handler);
  PrioritizedHandler prioritized = PrioritizedHandler();
  prioritized.handler = handler;
  prioritized.priority = priority;
  EventHandlerPriorityList::iterator it;
  switch (priority) {
    case Priority::kDefault:
      pre_target_list_.push_back(prioritized);
      return;
    case Priority::kSystem:
      // Find the beginning of the kSystem part of the list and prepend
      // this new handler to that section.
      // TODO(katie): We are adding this to the front of the list because
      // previously the function PrependPreTargetHandler added items to the
      // front in this way. See if we can simply put each item in the list and
      // sort, or insert each item the same way, in a later change.
      it = std::lower_bound(pre_target_list_.begin(), pre_target_list_.end(),
                            prioritized);
      pre_target_list_.insert(it, prioritized);
      return;
    case Priority::kAccessibility:
      pre_target_list_.insert(pre_target_list_.begin(), prioritized);
      return;
  }
}

void EventTarget::RemovePreTargetHandler(EventHandler* handler) {
  EventHandlerPriorityList::iterator it, end;
  for (it = pre_target_list_.begin(), end = pre_target_list_.end(); it != end;
       ++it) {
    if (it->handler == handler) {
      pre_target_list_.erase(it);
      return;
    }
  }
}

void EventTarget::AddPostTargetHandler(EventHandler* handler) {
  DCHECK(handler);
  post_target_list_.push_back(handler);
}

void EventTarget::RemovePostTargetHandler(EventHandler* handler) {
  EventHandlerList::iterator find =
      std::find(post_target_list_.begin(),
                post_target_list_.end(),
                handler);
  if (find != post_target_list_.end())
    post_target_list_.erase(find);
}

bool EventTarget::IsPreTargetListEmpty() const {
  return pre_target_list_.empty();
}

EventHandler* EventTarget::SetTargetHandler(EventHandler* target_handler) {
  EventHandler* original_target_handler = target_handler_;
  target_handler_ = target_handler;
  return original_target_handler;
}

// TODO(katie): trigger all kAccessibility handlers in the tree first,
// then kSystem and finally kDefault, rather than doing each set per
// parent level.
void EventTarget::GetPreTargetHandlers(EventHandlerList* list) {
  EventTarget* target = this;
  while (target) {
    EventHandlerPriorityList::reverse_iterator it, rend;
    for (it = target->pre_target_list_.rbegin(),
            rend = target->pre_target_list_.rend();
        it != rend;
        ++it) {
      list->insert(list->begin(), it->handler);
    }
    target = target->GetParentTarget();
  }
}

void EventTarget::GetPostTargetHandlers(EventHandlerList* list) {
  EventTarget* target = this;
  while (target) {
    for (EventHandlerList::iterator it = target->post_target_list_.begin(),
        end = target->post_target_list_.end(); it != end; ++it) {
      list->push_back(*it);
    }
    target = target->GetParentTarget();
  }
}

}  // namespace ui
