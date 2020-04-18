// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/win/rendering_window_manager.h"

#include "base/callback.h"
#include "base/memory/singleton.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"

namespace gfx {

// static
RenderingWindowManager* RenderingWindowManager::GetInstance() {
  return base::Singleton<RenderingWindowManager>::get();
}

void RenderingWindowManager::RegisterParent(HWND parent) {
  base::AutoLock lock(lock_);

  info_.emplace(parent, EmeddingInfo());
}

void SetParentAndMoveToBottom(HWND child, HWND parent) {
  ::SetParent(child, parent);
  // Move D3D window behind Chrome's window to avoid losing some messages.
  ::SetWindowPos(child, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

bool RenderingWindowManager::RegisterChild(HWND parent, HWND child) {
  if (!child)
    return false;

  base::AutoLock lock(lock_);

  auto it = info_.find(parent);
  if (it == info_.end())
    return false;

  EmeddingInfo& info = it->second;
  if (info.child)
    return false;

  info.child = child;

  // DoSetParentOnChild() was already called for |parent|. Call ::SetParent()
  // now but from a worker thread instead of the IO thread. The ::SetParent()
  // call could block the IO thread waiting on the UI thread and deadlock.
  if (info.call_set_parent) {
    base::PostTaskWithTraits(
        FROM_HERE, {base::TaskPriority::USER_BLOCKING},
        base::BindOnce(&SetParentAndMoveToBottom, child, parent));
  }

  return true;
}

void RenderingWindowManager::DoSetParentOnChild(HWND parent) {
  HWND child;
  {
    base::AutoLock lock(lock_);

    auto it = info_.find(parent);
    if (it == info_.end())
      return;

    EmeddingInfo& info = it->second;

    DCHECK(!info.call_set_parent);
    info.call_set_parent = true;

    // Call SetParentAndMoveToBottom() once RegisterChild() is called.
    if (!info.child)
      return;

    child = info.child;
  }

  SetParentAndMoveToBottom(child, parent);
}

void RenderingWindowManager::UnregisterParent(HWND parent) {
  base::AutoLock lock(lock_);
  info_.erase(parent);
}

bool RenderingWindowManager::HasValidChildWindow(HWND parent) {
  base::AutoLock lock(lock_);
  auto it = info_.find(parent);
  if (it == info_.end())
    return false;
  return !!it->second.child && ::IsWindow(it->second.child);
}

RenderingWindowManager::RenderingWindowManager() {}
RenderingWindowManager::~RenderingWindowManager() {}

}  // namespace gfx
