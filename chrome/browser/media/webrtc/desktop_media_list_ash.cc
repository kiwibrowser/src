// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/desktop_media_list_ash.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "chrome/grit/generated_resources.h"
#include "media/base/video_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"
#include "ui/snapshot/snapshot.h"

using content::DesktopMediaID;

namespace {

// Update the list twice per second.
const int kDefaultDesktopMediaListUpdatePeriod = 500;

}  // namespace

DesktopMediaListAsh::DesktopMediaListAsh(content::DesktopMediaID::Type type)
    : DesktopMediaListBase(base::TimeDelta::FromMilliseconds(
          kDefaultDesktopMediaListUpdatePeriod)),
      weak_factory_(this) {
  DCHECK(type == content::DesktopMediaID::TYPE_SCREEN ||
         type == content::DesktopMediaID::TYPE_WINDOW);
  type_ = type;
}

DesktopMediaListAsh::~DesktopMediaListAsh() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void DesktopMediaListAsh::Refresh() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::vector<SourceDescription> new_sources;
  EnumerateSources(&new_sources);

  UpdateSourcesList(new_sources);
}

void DesktopMediaListAsh::EnumerateWindowsForRoot(
    std::vector<DesktopMediaListAsh::SourceDescription>* sources,
    aura::Window* root_window,
    int container_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  aura::Window* container = ash::Shell::GetContainer(root_window, container_id);
  if (!container)
    return;
  for (aura::Window::Windows::const_iterator it = container->children().begin();
       it != container->children().end(); ++it) {
    if (!(*it)->IsVisible() || !(*it)->CanFocus())
      continue;
    content::DesktopMediaID id = content::DesktopMediaID::RegisterAuraWindow(
        content::DesktopMediaID::TYPE_WINDOW, *it);
    if (id.aura_id == view_dialog_id_.aura_id)
      continue;
    SourceDescription window_source(id, (*it)->GetTitle());
    sources->push_back(window_source);

    CaptureThumbnail(window_source.id, *it);
  }
}

void DesktopMediaListAsh::EnumerateSources(
    std::vector<DesktopMediaListAsh::SourceDescription>* sources) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  aura::Window::Windows root_windows = ash::Shell::GetAllRootWindows();

  for (size_t i = 0; i < root_windows.size(); ++i) {
    if (type_ == content::DesktopMediaID::TYPE_SCREEN) {
      SourceDescription screen_source(
          content::DesktopMediaID::RegisterAuraWindow(
              content::DesktopMediaID::TYPE_SCREEN, root_windows[i]),
          root_windows[i]->GetTitle());

      if (root_windows[i] == ash::Shell::GetPrimaryRootWindow())
        sources->insert(sources->begin(), screen_source);
      else
        sources->push_back(screen_source);

      if (screen_source.name.empty()) {
        if (root_windows.size() > 1) {
          // 'Screen' in 'Screen 1, Screen 2, etc ' might be inflected in some
          // languages depending on the number although rather unlikely. To be
          // safe, use the plural format.
          // TODO(jshin): Revert to GetStringFUTF16Int (with native digits)
          // if none of UI languages inflects 'Screen' in this context.
          screen_source.name = l10n_util::GetPluralStringFUTF16(
              IDS_DESKTOP_MEDIA_PICKER_MULTIPLE_SCREEN_NAME,
              static_cast<int>(i + 1));
        } else {
          screen_source.name = l10n_util::GetStringUTF16(
              IDS_DESKTOP_MEDIA_PICKER_SINGLE_SCREEN_NAME);
        }
      }

      CaptureThumbnail(screen_source.id, root_windows[i]);
    } else {
      EnumerateWindowsForRoot(
          sources, root_windows[i], ash::kShellWindowId_DefaultContainer);
      EnumerateWindowsForRoot(
          sources, root_windows[i], ash::kShellWindowId_AlwaysOnTopContainer);
    }
  }
}

void DesktopMediaListAsh::CaptureThumbnail(content::DesktopMediaID id,
                                           aura::Window* window) {
  gfx::Rect window_rect(window->bounds().width(), window->bounds().height());
  gfx::Rect scaled_rect = media::ComputeLetterboxRegion(
      gfx::Rect(thumbnail_size_), window_rect.size());

  ++pending_window_capture_requests_;
  ui::GrabWindowSnapshotAndScaleAsync(
      window, window_rect, scaled_rect.size(),
      base::Bind(&DesktopMediaListAsh::OnThumbnailCaptured,
                 weak_factory_.GetWeakPtr(), id));
}

void DesktopMediaListAsh::OnThumbnailCaptured(content::DesktopMediaID id,
                                              gfx::Image image) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  UpdateSourceThumbnail(id, image.AsImageSkia());

  --pending_window_capture_requests_;
  DCHECK_GE(pending_window_capture_requests_, 0);

  if (!pending_window_capture_requests_) {
    // Once we've finished capturing all windows post a task for the next list
    // update.
    ScheduleNextRefresh();
  }
}
