// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_THROBBER_H_
#define UI_VIEWS_CONTROLS_THROBBER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "ui/views/view.h"

namespace views {

// Throbbers display an animation, usually used as a status indicator.

class VIEWS_EXPORT Throbber : public View {
 public:
  Throbber();
  ~Throbber() override;

  // Start and stop the throbber animation.
  virtual void Start();
  virtual void Stop();

  // Stop spinning and, if checked is true, display a checkmark.
  void SetChecked(bool checked);

  // Overridden from View:
  gfx::Size CalculatePreferredSize() const override;
  void OnPaint(gfx::Canvas* canvas) override;

 protected:
  // Specifies whether the throbber is currently animating or not
  bool IsRunning() const;

 private:
  base::TimeTicks start_time_;  // Time when Start was called.
  base::RepeatingTimer timer_;  // Used to schedule Run calls.

  // Whether or not we should display a checkmark.
  bool checked_;

  DISALLOW_COPY_AND_ASSIGN(Throbber);
};

// A SmoothedThrobber is a throbber that is representing potentially short
// and nonoverlapping bursts of work.  SmoothedThrobber ignores small
// pauses in the work stops and starts, and only starts its throbber after
// a small amount of work time has passed.
class VIEWS_EXPORT SmoothedThrobber : public Throbber {
 public:
  SmoothedThrobber();
  ~SmoothedThrobber() override;

  void Start() override;
  void Stop() override;

  void set_start_delay_ms(int value) { start_delay_ms_ = value; }
  void set_stop_delay_ms(int value) { stop_delay_ms_ = value; }

 private:
  // Called when the startup-delay timer fires
  // This function starts the actual throbbing.
  void StartDelayOver();

  // Called when the shutdown-delay timer fires.
  // This function stops the actual throbbing.
  void StopDelayOver();

  // Delay after work starts before starting throbber, in milliseconds.
  int start_delay_ms_;

  // Delay after work stops before stopping, in milliseconds.
  int stop_delay_ms_;

  base::OneShotTimer start_timer_;
  base::OneShotTimer stop_timer_;

  DISALLOW_COPY_AND_ASSIGN(SmoothedThrobber);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_THROBBER_H_
