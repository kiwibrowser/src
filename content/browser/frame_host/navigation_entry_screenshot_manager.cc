// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/navigation_entry_screenshot_manager.h"

#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task_scheduler/post_task.h"
#include "content/browser/frame_host/navigation_controller_impl.h"
#include "content/browser/frame_host/navigation_entry_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/public/browser/overscroll_configuration.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/content_switches.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/effects/SkLumaColorFilter.h"
#include "ui/gfx/codec/png_codec.h"

namespace {

// Minimum delay between taking screenshots.
const int kMinScreenshotIntervalMS = 1000;

}

namespace content {

// Encodes the A8 SkBitmap to grayscale PNG in a worker thread.
class ScreenshotData : public base::RefCountedThreadSafe<ScreenshotData> {
 public:
  ScreenshotData() {
  }

  void EncodeScreenshot(const SkBitmap& bitmap, base::OnceClosure callback) {
    base::PostTaskWithTraitsAndReply(
        FROM_HERE, {base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
        base::BindOnce(&ScreenshotData::EncodeOnWorker, this, bitmap),
        std::move(callback));
  }

  scoped_refptr<base::RefCountedBytes> data() const { return data_; }

 private:
  friend class base::RefCountedThreadSafe<ScreenshotData>;
  virtual ~ScreenshotData() {
  }

  void EncodeOnWorker(const SkBitmap& bitmap) {
    // Convert |bitmap| to alpha-only |grayscale_bitmap|.
    SkBitmap grayscale_bitmap;
    if (grayscale_bitmap.tryAllocPixels(
            SkImageInfo::MakeA8(bitmap.width(), bitmap.height()))) {
      SkCanvas canvas(grayscale_bitmap);
#if defined(MEMORY_SANITIZER)
      // This is needed because Skia will operate over uninitialized memory
      // outside the visible region (with non-visible effects).
      canvas.clear(SK_ColorBLACK);
#endif
      SkPaint paint;
      paint.setColorFilter(SkLumaColorFilter::Make());
      canvas.drawBitmap(bitmap, SkIntToScalar(0), SkIntToScalar(0), &paint);
      canvas.flush();
    }
    if (!grayscale_bitmap.readyToDraw())
      return;

    // Encode the A8 bitmap to grayscale PNG treating alpha as color intensity.
    std::vector<unsigned char> data;
    if (gfx::PNGCodec::EncodeA8SkBitmap(grayscale_bitmap, &data))
      data_ = base::RefCountedBytes::TakeVector(&data);
  }

  scoped_refptr<base::RefCountedBytes> data_;

  DISALLOW_COPY_AND_ASSIGN(ScreenshotData);
};

NavigationEntryScreenshotManager::NavigationEntryScreenshotManager(
    NavigationControllerImpl* owner)
    : owner_(owner),
      min_screenshot_interval_ms_(kMinScreenshotIntervalMS),
      screenshot_factory_(this) {

}

NavigationEntryScreenshotManager::~NavigationEntryScreenshotManager() {
}

void NavigationEntryScreenshotManager::TakeScreenshot() {
  if (OverscrollConfig::GetHistoryNavigationMode() !=
      OverscrollConfig::HistoryNavigationMode::kParallaxUi) {
    return;
  }

  NavigationEntryImpl* entry = owner_->GetLastCommittedEntry();
  if (!entry)
    return;

  if (!owner_->delegate()->CanOverscrollContent())
    return;

  RenderViewHost* render_view_host = owner_->delegate()->GetRenderViewHost();
  DCHECK(render_view_host && render_view_host->GetWidget());
  content::RenderWidgetHostView* view =
      render_view_host->GetWidget()->GetView();
  if (!view)
    return;

  // Make sure screenshots aren't taken too frequently.
  base::Time now = base::Time::Now();
  if (now - last_screenshot_time_ <
          base::TimeDelta::FromMilliseconds(min_screenshot_interval_ms_)) {
    return;
  }

  WillTakeScreenshot(render_view_host);

  last_screenshot_time_ = now;

  // This screenshot is destined for the UI, so size the result to the actual
  // on-screen size of the view (and not its device-rendering size).
  const gfx::Size view_size_on_screen = view->GetViewBounds().size();
  view->CopyFromSurface(
      gfx::Rect(), view_size_on_screen,
      base::BindOnce(&NavigationEntryScreenshotManager::OnScreenshotTaken,
                     screenshot_factory_.GetWeakPtr(), entry->GetUniqueID()));
}

// Implemented here and not in NavigationEntry because this manager keeps track
// of the total number of screen shots across all entries.
void NavigationEntryScreenshotManager::ClearAllScreenshots() {
  int count = owner_->GetEntryCount();
  for (int i = 0; i < count; ++i) {
    ClearScreenshot(owner_->GetEntryAtIndex(i));
  }
  DCHECK_EQ(GetScreenshotCount(), 0);
}

void NavigationEntryScreenshotManager::SetMinScreenshotIntervalMS(
    int interval_ms) {
  DCHECK_GE(interval_ms, 0);
  min_screenshot_interval_ms_ = interval_ms;
}

void NavigationEntryScreenshotManager::OnScreenshotTaken(
    int unique_id,
    const SkBitmap& bitmap) {
  NavigationEntryImpl* entry = owner_->GetEntryWithUniqueID(unique_id);
  if (!entry) {
    LOG(ERROR) << "Invalid entry with unique id: " << unique_id;
    return;
  }

  if (bitmap.drawsNothing()) {
    if (!ClearScreenshot(entry))
      OnScreenshotSet(entry);
    return;
  }

  scoped_refptr<ScreenshotData> screenshot = new ScreenshotData();
  screenshot->EncodeScreenshot(
      bitmap, base::BindOnce(
                  &NavigationEntryScreenshotManager::OnScreenshotEncodeComplete,
                  screenshot_factory_.GetWeakPtr(), unique_id, screenshot));
}

int NavigationEntryScreenshotManager::GetScreenshotCount() const {
  int screenshot_count = 0;
  int entry_count = owner_->GetEntryCount();
  for (int i = 0; i < entry_count; ++i) {
    NavigationEntryImpl* entry = owner_->GetEntryAtIndex(i);
    if (entry->screenshot().get())
      screenshot_count++;
  }
  return screenshot_count;
}

void NavigationEntryScreenshotManager::OnScreenshotEncodeComplete(
    int unique_id,
    scoped_refptr<ScreenshotData> screenshot) {
  NavigationEntryImpl* entry = owner_->GetEntryWithUniqueID(unique_id);
  if (!entry)
    return;
  scoped_refptr<base::RefCountedBytes> data = screenshot->data();
  if (!data)
    return;
  entry->SetScreenshotPNGData(std::move(data));
  OnScreenshotSet(entry);
}

void NavigationEntryScreenshotManager::OnScreenshotSet(
    NavigationEntryImpl* entry) {
  if (entry->screenshot().get())
    PurgeScreenshotsIfNecessary();
}

bool NavigationEntryScreenshotManager::ClearScreenshot(
    NavigationEntryImpl* entry) {
  if (!entry->screenshot().get())
    return false;

  entry->SetScreenshotPNGData(nullptr);
  return true;
}

void NavigationEntryScreenshotManager::PurgeScreenshotsIfNecessary() {
  // Allow only a certain number of entries to keep screenshots.
  const int kMaxScreenshots = 10;
  int screenshot_count = GetScreenshotCount();
  if (screenshot_count < kMaxScreenshots)
    return;

  const int current = owner_->GetCurrentEntryIndex();
  const int num_entries = owner_->GetEntryCount();
  int available_slots = kMaxScreenshots;
  if (owner_->GetEntryAtIndex(current)->screenshot().get()) {
    --available_slots;
  }

  // Keep screenshots closer to the current navigation entry, and purge the ones
  // that are farther away from it. So in each step, look at the entries at
  // each offset on both the back and forward history, and start counting them
  // to make sure that the correct number of screenshots are kept in memory.
  // Note that it is possible for some entries to be missing screenshots (e.g.
  // when taking the screenshot failed for some reason). So there may be a state
  // where there are a lot of entries in the back history, but none of them has
  // any screenshot. In such cases, keep the screenshots for |kMaxScreenshots|
  // entries in the forward history list.
  int back = current - 1;
  int forward = current + 1;
  while (available_slots > 0 && (back >= 0 || forward < num_entries)) {
    if (back >= 0) {
      NavigationEntryImpl* entry = owner_->GetEntryAtIndex(back);
      if (entry->screenshot().get())
        --available_slots;
      --back;
    }

    if (available_slots > 0 && forward < num_entries) {
      NavigationEntryImpl* entry = owner_->GetEntryAtIndex(forward);
      if (entry->screenshot().get())
        --available_slots;
      ++forward;
    }
  }

  // Purge any screenshot at |back| or lower indices, and |forward| or higher
  // indices.
  while (screenshot_count > kMaxScreenshots && back >= 0) {
    NavigationEntryImpl* entry = owner_->GetEntryAtIndex(back);
    if (ClearScreenshot(entry))
      --screenshot_count;
    --back;
  }

  while (screenshot_count > kMaxScreenshots && forward < num_entries) {
    NavigationEntryImpl* entry = owner_->GetEntryAtIndex(forward);
    if (ClearScreenshot(entry))
      --screenshot_count;
    ++forward;
  }
  CHECK_GE(screenshot_count, 0);
  CHECK_LE(screenshot_count, kMaxScreenshots);
}

}  // namespace content
