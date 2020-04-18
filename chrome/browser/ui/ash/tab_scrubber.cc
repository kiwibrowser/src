// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/tab_scrubber.h"

#include <stdint.h>

#include <algorithm>

#include "ash/shell.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/glow_hover_controller.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "ui/aura/window.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/events/gesture_detection/gesture_configuration.h"

namespace {

const int64_t kActivationDelayMS = 200;

inline float Clamp(float value, float low, float high) {
  return std::min(high, std::max(value, low));
}

}  // namespace

// static
TabScrubber* TabScrubber::GetInstance() {
  static TabScrubber* instance = nullptr;
  if (!instance)
    instance = new TabScrubber();
  return instance;
}

// static
gfx::Point TabScrubber::GetStartPoint(TabStrip* tab_strip,
                                      int index,
                                      TabScrubber::Direction direction) {
  int initial_tab_offset = Tab::GetPinnedWidth() / 2;
  // In RTL layouts the tabs are mirrored. We hence use GetMirroredBounds()
  // which will give us the correct bounds of tabs in RTL layouts as well as
  // non-RTL layouts (in non-RTL layouts GetMirroredBounds() is the same as
  // bounds()).
  gfx::Rect tab_bounds = tab_strip->tab_at(index)->GetMirroredBounds();
  float x = direction == LEFT ? tab_bounds.x() + initial_tab_offset
                              : tab_bounds.right() - initial_tab_offset;
  return gfx::Point(x, tab_bounds.CenterPoint().y());
}

bool TabScrubber::IsActivationPending() {
  return activate_timer_.IsRunning();
}

TabScrubber::TabScrubber()
    : scrubbing_(false),
      browser_(nullptr),
      tab_strip_(nullptr),
      swipe_x_(-1),
      swipe_y_(-1),
      swipe_direction_(LEFT),
      highlighted_tab_(-1),
      activate_timer_(true, false),
      activation_delay_(kActivationDelayMS),
      use_default_activation_delay_(true),
      weak_ptr_factory_(this) {
  // TODO(mash): Add window server API to observe swipe gestures. Observing
  // gestures on browser windows is not sufficient, as this feature works when
  // the cursor is over the shelf, desktop, etc.
  ash::Shell::Get()->AddPreTargetHandler(this);
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_CLOSED,
                 content::NotificationService::AllSources());
}

TabScrubber::~TabScrubber() {
  // Note: The weak_ptr_factory_ should invalidate  its weak pointers before
  // any other members are destroyed.
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void TabScrubber::OnScrollEvent(ui::ScrollEvent* event) {
  if (event->type() == ui::ET_SCROLL_FLING_CANCEL ||
      event->type() == ui::ET_SCROLL_FLING_START) {
    FinishScrub(true);
    immersive_reveal_lock_.reset();
    return;
  }

  if (event->finger_count() != 3)
    return;

  Browser* browser = GetActiveBrowser();
  if (!browser || (scrubbing_ && browser_ && browser != browser_) ||
      (highlighted_tab_ != -1 &&
       highlighted_tab_ >= browser->tab_strip_model()->count())) {
    FinishScrub(false);
    return;
  }

  BrowserView* browser_view = BrowserView::GetBrowserViewForNativeWindow(
      browser->window()->GetNativeWindow());
  TabStrip* tab_strip = browser_view->tabstrip();

  if (tab_strip->IsAnimating()) {
    FinishScrub(false);
    return;
  }

  // We are handling the event.
  event->StopPropagation();

  // The event's x_offset doesn't change in an RTL layout. Negative value means
  // left, positive means right.
  float x_offset = event->x_offset();
  int initial_tab_index = highlighted_tab_ == -1
                              ? browser->tab_strip_model()->active_index()
                              : highlighted_tab_;
  if (!scrubbing_) {
    BeginScrub(browser, browser_view, x_offset);
  } else if (highlighted_tab_ == -1) {
    // Has the direction of the swipe changed while scrubbing?
    Direction direction = (x_offset < 0) ? LEFT : RIGHT;
    if (direction != swipe_direction_)
      ScrubDirectionChanged(direction);
  }

  UpdateSwipeX(x_offset);

  Tab* initial_tab = tab_strip_->tab_at(initial_tab_index);
  gfx::Point tab_point(swipe_x_, swipe_y_);
  views::View::ConvertPointToTarget(tab_strip_, initial_tab, &tab_point);
  Tab* new_tab = tab_strip_->GetTabAt(initial_tab, tab_point);
  if (!new_tab)
    return;

  int new_index = tab_strip_->GetModelIndexOfTab(new_tab);
  if (highlighted_tab_ == -1 &&
      new_index == browser_->tab_strip_model()->active_index()) {
    return;
  }

  if (new_index != highlighted_tab_) {
    if (activate_timer_.IsRunning())
      activate_timer_.Reset();
    else
      ScheduleFinishScrubIfNeeded();
  }

  UpdateHighlightedTab(new_tab, new_index);

  if (highlighted_tab_ != -1) {
    gfx::Point hover_point(swipe_x_, swipe_y_);
    views::View::ConvertPointToTarget(tab_strip_, new_tab, &hover_point);
    new_tab->hover_controller()->SetLocation(hover_point);
  }
}

void TabScrubber::Observe(int type,
                          const content::NotificationSource& source,
                          const content::NotificationDetails& details) {
  if (content::Source<Browser>(source).ptr() == browser_) {
    activate_timer_.Stop();
    swipe_x_ = -1;
    swipe_y_ = -1;
    scrubbing_ = false;
    highlighted_tab_ = -1;
    browser_ = nullptr;
    tab_strip_ = nullptr;
  }
}

void TabScrubber::OnTabAdded(int index) {
  if (highlighted_tab_ == -1)
    return;

  if (index < highlighted_tab_)
    ++highlighted_tab_;
}

void TabScrubber::OnTabMoved(int from_index, int to_index) {
  if (highlighted_tab_ == -1)
    return;

  if (from_index == highlighted_tab_)
    highlighted_tab_ = to_index;
  else if (from_index < highlighted_tab_ && highlighted_tab_ <= to_index)
    --highlighted_tab_;
  else if (from_index > highlighted_tab_ && highlighted_tab_ >= to_index)
    ++highlighted_tab_;
}

void TabScrubber::OnTabRemoved(int index) {
  if (highlighted_tab_ == -1)
    return;
  if (index == highlighted_tab_) {
    FinishScrub(false);
    return;
  }
  if (index < highlighted_tab_)
    --highlighted_tab_;
}

Browser* TabScrubber::GetActiveBrowser() {
  Browser* browser = chrome::FindLastActive();
  if (!browser || browser->type() != Browser::TYPE_TABBED ||
      !browser->window()->IsActive()) {
    return nullptr;
  }

  return browser;
}

void TabScrubber::BeginScrub(Browser* browser,
                             BrowserView* browser_view,
                             float x_offset) {
  DCHECK(browser);
  DCHECK(browser_view);

  tab_strip_ = browser_view->tabstrip();
  scrubbing_ = true;
  browser_ = browser;

  Direction direction = (x_offset < 0) ? LEFT : RIGHT;
  ScrubDirectionChanged(direction);

  ImmersiveModeController* immersive_controller =
      browser_view->immersive_mode_controller();
  if (immersive_controller->IsEnabled()) {
    immersive_reveal_lock_.reset(immersive_controller->GetRevealedLock(
        ImmersiveModeController::ANIMATE_REVEAL_YES));
  }

  tab_strip_->AddObserver(this);
}

void TabScrubber::FinishScrub(bool activate) {
  activate_timer_.Stop();

  if (browser_ && browser_->window()) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForNativeWindow(
        browser_->window()->GetNativeWindow());
    TabStrip* tab_strip = browser_view->tabstrip();
    if (activate && highlighted_tab_ != -1) {
      Tab* tab = tab_strip->tab_at(highlighted_tab_);
      tab->hover_controller()->HideImmediately();
      int distance = std::abs(highlighted_tab_ -
                              browser_->tab_strip_model()->active_index());
      UMA_HISTOGRAM_CUSTOM_COUNTS("Tabs.ScrubDistance", distance, 1, 20, 21);
      browser_->tab_strip_model()->ActivateTabAt(highlighted_tab_, true);
    }
    tab_strip->RemoveObserver(this);
  }

  browser_ = nullptr;
  tab_strip_ = nullptr;
  swipe_x_ = -1;
  swipe_y_ = -1;
  scrubbing_ = false;
  highlighted_tab_ = -1;
}

void TabScrubber::ScheduleFinishScrubIfNeeded() {
  int delay = use_default_activation_delay_
                  ? ui::GestureConfiguration::GetInstance()
                        ->tab_scrub_activation_delay_in_ms()
                  : activation_delay_;

  if (delay >= 0) {
    activate_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(delay),
                          base::Bind(&TabScrubber::FinishScrub,
                                     weak_ptr_factory_.GetWeakPtr(), true));
  }
}

void TabScrubber::ScrubDirectionChanged(Direction direction) {
  DCHECK(browser_);
  DCHECK(tab_strip_);
  DCHECK(scrubbing_);

  swipe_direction_ = direction;
  const gfx::Point start_point =
      GetStartPoint(tab_strip_, browser_->tab_strip_model()->active_index(),
                    swipe_direction_);
  swipe_x_ = start_point.x();
  swipe_y_ = start_point.y();
}

void TabScrubber::UpdateSwipeX(float x_offset) {
  DCHECK(browser_);
  DCHECK(tab_strip_);
  DCHECK(scrubbing_);

  // Make the swipe speed inversely proportional with the number or tabs:
  // Each added tab introduces a reduction of 2% in |x_offset|, with a value of
  // one fourth of |x_offset| as the minimum (i.e. we need 38 tabs to reach
  // that minimum reduction).
  swipe_x_ += Clamp(x_offset - (tab_strip_->tab_count() * 0.02f * x_offset),
                    0.25f * x_offset, x_offset);

  // In an RTL layout, everything is mirrored, i.e. the index of the first tab
  // (with the smallest X mirrored co-ordinates) is actually the index of the
  // last tab. Same for the index of the last tab.
  int first_tab_index = base::i18n::IsRTL() ? tab_strip_->tab_count() - 1 : 0;
  int last_tab_index = base::i18n::IsRTL() ? 0 : tab_strip_->tab_count() - 1;

  Tab* first_tab = tab_strip_->tab_at(first_tab_index);
  int first_tab_center = first_tab->GetMirroredBounds().CenterPoint().x();
  Tab* last_tab = tab_strip_->tab_at(last_tab_index);
  int last_tab_center = last_tab->GetMirroredBounds().CenterPoint().x();

  if (swipe_x_ < first_tab_center)
    swipe_x_ = first_tab_center;
  if (swipe_x_ > last_tab_center)
    swipe_x_ = last_tab_center;
}

void TabScrubber::UpdateHighlightedTab(Tab* new_tab, int new_index) {
  DCHECK(scrubbing_);
  DCHECK(new_tab);

  if (new_index == highlighted_tab_)
    return;

  if (highlighted_tab_ != -1) {
    Tab* tab = tab_strip_->tab_at(highlighted_tab_);
    tab->hover_controller()->HideImmediately();
  }

  if (new_index != browser_->tab_strip_model()->active_index()) {
    highlighted_tab_ = new_index;
    new_tab->hover_controller()->Show(GlowHoverController::PRONOUNCED);
  } else {
    highlighted_tab_ = -1;
  }
}
