// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_THREADED_IMAGE_CURSORS_H_
#define SERVICES_UI_WS_THREADED_IMAGE_CURSORS_H_

#include "base/memory/weak_ptr.h"
#include "ui/base/cursor/cursor.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace display {
class Display;
}

namespace ui {
class CursorData;
class ImageCursors;
class ImageCursorsSet;
class PlatformWindow;

namespace ws {

// Wrapper around ui::ImageCursors, which executes its methods asynchronously
// via the provided task runner. This is needed because ui::ImageCursors needs
// to load resources when executing some of its methods, which cannot happen on
// the UI Service's thread when it runs inside the Window Manager process.
//
// Unlike ImageCursors, this class can also handle CursorType::kCustom cursors,
// which is also done asynchronously.
//
// Note: We could execute the methods synchronously when UI Service runs in its
// own process (and thus the provided task runner matches the task runner of the
// UI Service's thread), but we don't do that in the interest of sharing code.
class ThreadedImageCursors {
 public:
  // |resource_runner| is the task runner for the thread which can be used to
  // load resources; |image_cursors_set_weak_ptr_| points to an object that
  // lives on |resource_runner|'s thread.
  // We create an ImageCursors object here and then pass the ownership to
  // |image_cursors_set_weak_ptr_|. All operations on the ImageCursors object
  // happen on |resource_runner|, which is why we need the owner to live on
  // |resource_runner|'s thread.
  ThreadedImageCursors(
      scoped_refptr<base::SingleThreadTaskRunner>& resource_task_runner,
      base::WeakPtr<ui::ImageCursorsSet> image_cursors_set_weak_ptr_);
  ~ThreadedImageCursors();

  // Executes ui::ImageCursors::SetDisplay asynchronously.
  // Sets the display the cursors are loaded for. |scale_factor| determines the
  // size of the image to load. Returns true if the cursor image is reloaded.
  void SetDisplay(const display::Display& display, float scale_factor);

  // Asynchronously sets the size of the mouse cursor icon.
  void SetCursorSize(CursorSize cursor_size);

  // Asynchronously loads the cursor and then sets the corresponding
  // PlatformCursor on the provided |platform_window|.
  // |platform_window| pointer needs to be valid while this object is alive.
  void SetCursor(const ui::CursorData& cursor_data,
                 ui::PlatformWindow* platform_window);

  // Helper method. Sets |platform_cursor| on the |platform_window|.
  void SetCursorOnPlatformWindow(ui::PlatformCursor platform_cursor,
                                 ui::PlatformWindow* platform_window);
#if defined(USE_OZONE)
  void SetCursorOnPlatformWindowAndUnref(ui::PlatformCursor platform_cursor,
                                         ui::PlatformWindow* platform_window);
#endif

 private:
  // The object used for performing the actual cursor operations.
  // Created on UI Service's thread, but is used on the
  // |resource_task_runner_|'s thread, because it needs to load resources.
  base::WeakPtr<ui::ImageCursors> image_cursors_weak_ptr_;

  // Task runner for the thread which owns the cursor resources.
  // |image_cursors_set_weak_ptr__| and |image_cursors_weak_ptr_| should only
  // be dereferenced on this task runner.
  scoped_refptr<base::SingleThreadTaskRunner> resource_task_runner_;

  // Task runner of the UI Service thread (where this object is created and
  // destroyed).
  scoped_refptr<base::SingleThreadTaskRunner> ui_service_task_runner_;

  // Lives on |resource_runner_|'s thread, used to own the object behind
  // |image_cursors_weak_ptr_|.
  base::WeakPtr<ui::ImageCursorsSet> image_cursors_set_weak_ptr_;

  base::WeakPtrFactory<ThreadedImageCursors> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThreadedImageCursors);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_THREADED_IMAGE_CURSORS_H_
