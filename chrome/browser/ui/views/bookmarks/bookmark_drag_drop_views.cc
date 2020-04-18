// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/bookmarks/bookmark_drag_drop.h"

#include "base/message_loop/message_loop_current.h"
#include "build/build_config.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views_mode_controller.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node_data.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/views/drag_utils.h"
#include "ui/views/widget/widget.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace chrome {

void DragBookmarks(Profile* profile,
                   const std::vector<const BookmarkNode*>& nodes,
                   gfx::NativeView view,
                   ui::DragDropTypes::DragEventSource source) {
#if defined(OS_MACOSX)
  if (views_mode_controller::IsViewsBrowserCocoa())
    return DragBookmarksCocoa(profile, nodes, view, source);
#endif
  DCHECK(!nodes.empty());

  // Set up our OLE machinery.
  ui::OSExchangeData data;
  bookmarks::BookmarkNodeData drag_data(nodes);
  drag_data.Write(profile->GetPath(), &data);

  // Allow nested run loop so we get DnD events as we drag this around.
  base::MessageLoopCurrent::ScopedNestableTaskAllower nestable_task_allower;

  int operation = ui::DragDropTypes::DRAG_COPY | ui::DragDropTypes::DRAG_LINK;
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile);
  if (bookmarks::CanAllBeEditedByUser(model->client(), nodes))
    operation |= ui::DragDropTypes::DRAG_MOVE;

  views::Widget* widget = views::Widget::GetWidgetForNativeView(view);

  if (widget) {
    widget->RunShellDrag(NULL, data, gfx::Point(), operation, source);
  } else {
    // We hit this case when we're using WebContentsViewWin or
    // WebContentsViewAura, instead of WebContentsViewViews.
    views::RunShellDrag(view, data, gfx::Point(), operation, source);
  }
}

}  // namespace chrome
