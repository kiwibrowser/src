// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/threaded_image_cursors.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/ui/common/image_cursors_set.h"
#include "ui/base/cursor/cursor_data.h"
#include "ui/base/cursor/image_cursors.h"
#include "ui/platform_window/platform_window.h"

#if defined(USE_OZONE)
#include "ui/ozone/public/cursor_factory_ozone.h"
#include "ui/ozone/public/ozone_platform.h"
#endif

namespace ui {
namespace ws {
namespace {

void AddImageCursorsOnResourceThread(
    base::WeakPtr<ui::ImageCursorsSet> image_cursors_set_weak_ptr,
    std::unique_ptr<ui::ImageCursors> image_cursors) {
  if (image_cursors_set_weak_ptr)
    image_cursors_set_weak_ptr->AddImageCursors(std::move(image_cursors));
}

void RemoveImageCursorsOnResourceThread(
    base::WeakPtr<ui::ImageCursorsSet> image_cursors_set_weak_ptr,
    base::WeakPtr<ui::ImageCursors> image_cursors_weak_ptr) {
  if (image_cursors_set_weak_ptr && image_cursors_weak_ptr) {
    image_cursors_set_weak_ptr->DeleteImageCursors(
        image_cursors_weak_ptr.get());
  }
}

void SetDisplayOnResourceThread(
    base::WeakPtr<ui::ImageCursors> image_cursors_weak_ptr,
    const display::Display& display,
    float scale_factor) {
  if (image_cursors_weak_ptr)
    image_cursors_weak_ptr->SetDisplay(display, scale_factor);
}

void SetCursorSizeOnResourceThread(
    base::WeakPtr<ui::ImageCursors> image_cursors_weak_ptr,
    CursorSize cursor_size) {
  if (image_cursors_weak_ptr)
    image_cursors_weak_ptr->SetCursorSize(cursor_size);
}

// Executed on |resource_task_runner_|. Sets cursor type on
// |image_cursors_set_weak_ptr_|, and then schedules a task on
// |ui_service_task_runner_| to set the corresponding PlatformCursor on the
// provided |platform_window|.
// |platform_window| pointer needs to be valid while
// |threaded_image_cursors_weak_ptr| is not invalidated.
void SetCursorOnResourceThread(
    base::WeakPtr<ui::ImageCursors> image_cursors_weak_ptr,
    ui::CursorType cursor_type,
    ui::PlatformWindow* platform_window,
    scoped_refptr<base::SingleThreadTaskRunner> ui_service_task_runner_,
    base::WeakPtr<ThreadedImageCursors> threaded_image_cursors_weak_ptr) {
  if (image_cursors_weak_ptr) {
    ui::Cursor native_cursor(cursor_type);
    image_cursors_weak_ptr->SetPlatformCursor(&native_cursor);
    // |platform_window| is owned by the UI Service thread, so setting the
    // cursor on it also needs to happen on that thread.
    ui_service_task_runner_->PostTask(
        FROM_HERE, base::Bind(&ThreadedImageCursors::SetCursorOnPlatformWindow,
                              threaded_image_cursors_weak_ptr,
                              native_cursor.platform(), platform_window));
  }
}

#if defined(USE_OZONE)
// Executed on |resource_task_runner_|. Creates a  PlatformCursor using the
// Ozone |cursor_factory| passed to it, and then schedules a task on
// |ui_service_task_runner_| to set that cursor on the provided
// |platform_window|.
// |platform_window| pointer needs to be valid while
// |threaded_image_cursors_weak_ptr| is not invalidated.
void SetCustomCursorOnResourceThread(
    base::WeakPtr<ui::ImageCursors> image_cursors_weak_ptr,
    std::unique_ptr<ui::CursorData> cursor_data,
    ui::CursorFactoryOzone* cursor_factory,
    ui::PlatformWindow* platform_window,
    scoped_refptr<base::SingleThreadTaskRunner> ui_service_task_runner_,
    base::WeakPtr<ThreadedImageCursors> threaded_image_cursors_weak_ptr) {
  if (image_cursors_weak_ptr) {
    ui::PlatformCursor platform_cursor = cursor_factory->CreateAnimatedCursor(
        cursor_data->cursor_frames(), cursor_data->hotspot_in_pixels(),
        cursor_data->frame_delay().InMilliseconds(),
        cursor_data->scale_factor());
    // |platform_window| is owned by the UI Service thread, so setting the
    // cursor on it also needs to happen on that thread.
    ui_service_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&ThreadedImageCursors::SetCursorOnPlatformWindowAndUnref,
                   threaded_image_cursors_weak_ptr, platform_cursor,
                   platform_window));
  }
}
#endif  // defined(USE_OZONE)

}  // namespace

ThreadedImageCursors::ThreadedImageCursors(
    scoped_refptr<base::SingleThreadTaskRunner>& resource_task_runner,
    base::WeakPtr<ui::ImageCursorsSet> image_cursors_set_weak_ptr)
    : resource_task_runner_(resource_task_runner),
      image_cursors_set_weak_ptr_(image_cursors_set_weak_ptr),
      weak_ptr_factory_(this) {
  DCHECK(resource_task_runner_);
  ui_service_task_runner_ = base::ThreadTaskRunnerHandle::Get();

  // Create and initialize the ImageCursors object here and then set it on
  // |image_cursors_set_weak_ptr_|. Note that it is essential to initialize
  // the ImageCursors object on the UI Service's thread if we are using Ozone,
  // so that it uses the right (thread-local) CursorFactoryOzone instance.
  std::unique_ptr<ui::ImageCursors> image_cursors =
      std::make_unique<ui::ImageCursors>();
  image_cursors->Initialize();
  image_cursors_weak_ptr_ = image_cursors->GetWeakPtr();
  resource_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&AddImageCursorsOnResourceThread, image_cursors_set_weak_ptr_,
                 base::Passed(std::move(image_cursors))));
}

ThreadedImageCursors::~ThreadedImageCursors() {
  resource_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RemoveImageCursorsOnResourceThread,
                 image_cursors_set_weak_ptr_, image_cursors_weak_ptr_));
}

void ThreadedImageCursors::SetDisplay(const display::Display& display,
                                      float scale_factor) {
  resource_task_runner_->PostTask(
      FROM_HERE, base::Bind(&SetDisplayOnResourceThread,
                            image_cursors_weak_ptr_, display, scale_factor));
}

void ThreadedImageCursors::SetCursorSize(CursorSize cursor_size) {
  resource_task_runner_->PostTask(
      FROM_HERE, base::Bind(&SetCursorSizeOnResourceThread,
                            image_cursors_weak_ptr_, cursor_size));
}

void ThreadedImageCursors::SetCursor(const ui::CursorData& cursor_data,
                                     ui::PlatformWindow* platform_window) {
  ui::CursorType cursor_type = cursor_data.cursor_type();

#if !defined(USE_OZONE)
  // Outside of ozone builds, there isn't a single interface for creating
  // PlatformCursors. The closest thing to one is in //content/ instead of
  // //ui/ which means we can't use it from here. For now, just don't handle
  // custom image cursors.
  //
  // TODO(erg): Once blink speaks directly to mus, make blink perform its own
  // cursor management on its own mus windows so we can remove Webcursor from
  // //content/ and do this in way that's safe cross-platform, instead of as an
  // ozone-specific hack.
  if (cursor_type == ui::CursorType::kCustom) {
    NOTIMPLEMENTED() << "No custom cursor support on non-ozone yet.";
    cursor_type = ui::CursorType::kPointer;
  }
#else
  // In Ozone builds, we have an interface available which turns bitmap data
  // into platform cursors.
  if (cursor_type == ui::CursorType::kCustom) {
    std::unique_ptr<ui::CursorData> cursor_data_copy =
        std::make_unique<ui::CursorData>(cursor_data);
    ui::CursorFactoryOzone* cursor_factory =
        ui::CursorFactoryOzone::GetInstance();
    // CursorFactoryOzone is a thread-local singleton (crbug.com/741106).
    // However the instance that belongs to the UI Service thread is used
    // on the resource thread. (This happens via ImageCursors when we call
    // SetCursorOnResourceThread.) Since CursorFactoryOzone is not thread-safe,
    // we should only use it on the UI Service thread, which is why this
    // PostTask is needed.
    resource_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SetCustomCursorOnResourceThread, image_cursors_weak_ptr_,
                   base::Passed(&cursor_data_copy),
                   base::Unretained(cursor_factory), platform_window,
                   ui_service_task_runner_, weak_ptr_factory_.GetWeakPtr()));
  }
#endif  // !defined(USE_OZONE)

  // Handle the non-custom cursor case for both Ozone and non-Ozone builds.
  if (cursor_type != ui::CursorType::kCustom) {
    resource_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SetCursorOnResourceThread, image_cursors_weak_ptr_,
                   cursor_type, platform_window, ui_service_task_runner_,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void ThreadedImageCursors::SetCursorOnPlatformWindow(
    ui::PlatformCursor platform_cursor,
    ui::PlatformWindow* platform_window) {
  platform_window->SetCursor(platform_cursor);
}

#if defined(USE_OZONE)
void ThreadedImageCursors::SetCursorOnPlatformWindowAndUnref(
    ui::PlatformCursor platform_cursor,
    ui::PlatformWindow* platform_window) {
  SetCursorOnPlatformWindow(platform_cursor, platform_window);

  // The PlatformCursor returned by CreateAnimatedCursor() has a single
  // refcount which is owned by the caller. If we were returned a valid
  // |platform_cursor|, we must unref it here. All implementations of
  // PlatformWindow::SetCursor() which need to keep the PlatformCursor have
  // increased the refcount for their self.
  //
  // Note: Cursors returned by SetPlatformCursor() do not return custom bitmap
  // cursors. The refcount of such a cursor is owned by the caller and
  // shouldn't be refed/unrefed by the caller.
  if (platform_cursor)
    ui::CursorFactoryOzone::GetInstance()->UnrefImageCursor(platform_cursor);
}
#endif

}  // namespace ws
}  // namespace ui
