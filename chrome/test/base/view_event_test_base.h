// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_VIEW_EVENT_TEST_BASE_H_
#define CHROME_TEST_BASE_VIEW_EVENT_TEST_BASE_H_

// We only want to use ViewEventTestBase in test targets which properly
// isolate each test case by running each test in a separate process.
// This way if a test hangs the test launcher can reliably terminate it.
#if defined(HAS_OUT_OF_PROC_TEST_RUNNER)

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "chrome/test/views/chrome_test_views_delegate.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/widget/widget_delegate.h"

#if defined(OS_WIN)
#include "ui/base/win/scoped_ole_initializer.h"
#endif

namespace gfx {
class Size;
}

class ViewEventTestPlatformPart;

// Base class for Views based tests that dispatch events.
//
// As views based event test involves waiting for events to be processed,
// writing a views based test is slightly different than that of writing
// other unit tests. In particular when the test fails or is done you need
// to stop the message loop. This can be done by way of invoking the Done
// method.
//
// Any delayed callbacks should be done by way of CreateEventTask.
// CreateEventTask checks to see if ASSERT_XXX has been invoked after invoking
// the task. If there was a failure Done is invoked and the test stops.
//
// ViewEventTestBase creates a Window with the View returned from
// CreateContentsView. The preferred size for the view can be customized by
// overriding GetPreferredSizeForContents. If you do not override
// GetPreferredSizeForContents the preferred size of the view returned from
// CreateContentsView is used.
//
// Subclasses of ViewEventTestBase must implement two methods:
// . DoTestOnMessageLoop: invoked when the message loop is running. Run your
//   test here, invoke Done when done.
// . CreateContentsView: returns the view to place in the window.
//
// Once you have created a ViewEventTestBase use the macro VIEW_TEST to define
// the fixture.
//
// I encountered weird timing problems in initiating dragging and drop that
// necessitated ugly hacks. In particular when the hook installed by
// ui_controls received the mouse event and posted a task that task was not
// processed. To work around this use the following pattern when initiating
// dnd:
//   // Schedule the mouse move at a location slightly different from where
//   // you really want to move to.
//   ui_controls::SendMouseMoveNotifyWhenDone(loc.x + 10, loc.y,
//       base::BindOnce(&YYY, this));
//   // Then use this to schedule another mouse move.
//   ScheduleMouseMoveInBackground(loc.x, loc.y);

class ViewEventTestBase : public views::WidgetDelegate,
                          public testing::Test {
 public:
  ViewEventTestBase();

  // Invoke when done either because of failure or success. Quits the message
  // loop.
  void Done();

  static void SetUpTestCase();

  // Creates a window.
  void SetUp() override;

  // Destroys the window.
  void TearDown() override;

  // Returns an empty Size. Subclasses that want a preferred size other than
  // that of the View returned by CreateContentsView should override this
  // appropriately.
  virtual gfx::Size GetPreferredSizeForContents() const;

  // Overridden from views::WidgetDelegate:
  bool CanResize() const override;
  views::View* GetContentsView() override;
  const views::Widget* GetWidget() const override;
  views::Widget* GetWidget() override;

  // Overridden to do nothing so that this class can be used in runnable tasks.
  void AddRef() {}
  void Release() {}

 protected:
  ~ViewEventTestBase() override;

  // Returns the view that is added to the window.
  virtual views::View* CreateContentsView() = 0;

  // Called once the message loop is running.
  virtual void DoTestOnMessageLoop() = 0;

  // Invoke from test main. Shows the window, starts the message loop and
  // schedules a task that invokes DoTestOnMessageLoop.
  void StartMessageLoopAndRunTest();

  // Creates a task that calls the specified method back. The specified
  // method is called in such a way that if there are any test failures
  // Done is invoked.
  template <class T, class Method>
  base::Closure CreateEventTask(T* target, Method method) {
    return base::Bind(&ViewEventTestBase::RunTestMethod, this,
                      base::Bind(method, target));
  }

  // Spawns a new thread posts a MouseMove in the background.
  void ScheduleMouseMoveInBackground(int x, int y);

  views::Widget* window_;

 private:
  // Stops the thread started by ScheduleMouseMoveInBackground.
  void StopBackgroundThread();

  // Callback from CreateEventTask. Stops the background thread, runs the
  // supplied task and if there are failures invokes Done.
  void RunTestMethod(const base::Closure& task);

  // The content of the Window.
  views::View* content_view_;

  // Thread for posting background MouseMoves.
  std::unique_ptr<base::Thread> dnd_thread_;

  content::TestBrowserThreadBundle thread_bundle_;

#if defined(OS_WIN)
  ui::ScopedOleInitializer ole_initializer_;
#endif

  std::unique_ptr<ViewEventTestPlatformPart> platform_part_;

  ChromeTestViewsDelegate views_delegate_;

  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(ViewEventTestBase);
};

// Convenience macro for defining a ViewEventTestBase. See class description
// of ViewEventTestBase for details.
#define VIEW_TEST(test_class, name) \
  TEST_F(test_class, name) {\
    StartMessageLoopAndRunTest();\
  }

#endif  // defined(HAS_OUT_OF_PROC_TEST_RUNNER)

#endif  // CHROME_TEST_BASE_VIEW_EVENT_TEST_BASE_H_
