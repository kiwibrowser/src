// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_TAB_SCRUBBER_H_
#define CHROME_BROWSER_UI_ASH_TAB_SCRUBBER_H_

#include <memory>

#include "base/macros.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/views/frame/immersive_mode_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/events/event_handler.h"

class Browser;
class Tab;
class TabStrip;

namespace gfx {
class Point;
}

// Class to enable quick tab switching via horizontal 3 finger swipes.
class TabScrubber : public ui::EventHandler,
                    public content::NotificationObserver,
                    public TabStripObserver {
 public:
  enum Direction { LEFT, RIGHT };

  // Returns a the single instance of a TabScrubber.
  static TabScrubber* GetInstance();

  // Returns the virtual position of a swipe starting in the tab at |index|,
  // base on the |direction|.
  static gfx::Point GetStartPoint(TabStrip* tab_strip,
                                  int index,
                                  TabScrubber::Direction direction);

  void set_activation_delay(int activation_delay) {
    activation_delay_ = activation_delay;
    use_default_activation_delay_ = false;
  }
  int activation_delay() const { return activation_delay_; }
  int highlighted_tab() const { return highlighted_tab_; }
  bool IsActivationPending();

 private:
  TabScrubber();
  ~TabScrubber() override;

  // ui::EventHandler overrides:
  void OnScrollEvent(ui::ScrollEvent* event) override;

  // content::NotificationObserver overrides:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // TabStripObserver overrides.
  void OnTabAdded(int index) override;
  void OnTabMoved(int from_index, int to_index) override;
  void OnTabRemoved(int index) override;

  Browser* GetActiveBrowser();

  void BeginScrub(Browser* browser, BrowserView* browser_view, float x_offset);
  void FinishScrub(bool activate);

  void ScheduleFinishScrubIfNeeded();

  // Updates the direction and the starting point of the swipe.
  void ScrubDirectionChanged(Direction direction);

  // Updates the X co-ordinate of the swipe taking into account RTL layouts if
  // any.
  void UpdateSwipeX(float x_offset);

  void UpdateHighlightedTab(Tab* new_tab, int new_index);

  // Are we currently scrubbing?.
  bool scrubbing_;
  // The last browser we used for scrubbing, NULL if |scrubbing_| is
  // false and there is no pending work.
  Browser* browser_;
  // The TabStrip of the active browser we're scrubbing.
  TabStrip* tab_strip_;
  // The current accumulated x and y positions of a swipe, in
  // the coordinates of the TabStrip of |browser_|
  float swipe_x_;
  float swipe_y_;
  // The direction the current swipe is headed.
  Direction swipe_direction_;
  // The index of the tab that is currently highlighted.
  int highlighted_tab_;
  // Timer to control a delayed activation of the |highlighted_tab_|.
  base::Timer activate_timer_;
  // Time to wait in ms before newly selected tab becomes active.
  int activation_delay_;
  // Set if activation_delay had been explicitly set.
  bool use_default_activation_delay_;
  // Forces the tabs to be revealed if we are in immersive fullscreen.
  std::unique_ptr<ImmersiveRevealedLock> immersive_reveal_lock_;

  content::NotificationRegistrar registrar_;
  base::WeakPtrFactory<TabScrubber> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TabScrubber);
};

#endif  // CHROME_BROWSER_UI_ASH_TAB_SCRUBBER_H_
