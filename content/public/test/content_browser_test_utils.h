// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_CONTENT_BROWSER_TEST_UTILS_H_
#define CONTENT_PUBLIC_TEST_CONTENT_BROWSER_TEST_UTILS_H_

#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/common/page_type.h"
#include "ui/gfx/native_widget_types.h"
#include "url/gurl.h"

namespace base {
class FilePath;

namespace mac {
class ScopedObjCClassSwizzler;
}
}

namespace gfx {
class Point;
class Range;
class Rect;
}

// A collections of functions designed for use with content_shell based browser
// tests.
// Note: if a function here also works with browser_tests, it should be in
// content\public\test\browser_test_utils.h

namespace content {

class MessageLoopRunner;
class RenderFrameHost;
class RenderWidgetHost;
class Shell;
class ToRenderFrameHost;
class WebContents;

// Generate the file path for testing a particular test.
// The file for the tests is all located in
// content/test/data/dir/<file>
// The returned path is FilePath format.
//
// A null |dir| indicates the root directory - i.e.
// content/test/data/<file>
base::FilePath GetTestFilePath(const char* dir, const char* file);

// Generate the URL for testing a particular test.
// HTML for the tests is all located in
// test_root_directory/dir/<file>
// The returned path is GURL format.
//
// A null |dir| indicates the root directory - i.e.
// content/test/data/<file>
GURL GetTestUrl(const char* dir, const char* file);

// Navigates |window| to |url|, blocking until the navigation finishes. Returns
// true if the page was loaded successfully and the last committed URL matches
// |url|.  This is a browser-initiated navigation that simulates a user typing
// |url| into the address bar.
//
// TODO(alexmos): any tests that use this function and expect successful
// navigations should do EXPECT_TRUE(NavigateToURL()).
bool NavigateToURL(Shell* window, const GURL& url);

// Performs a renderer-initiated navigation of |window| to |url|, blocking
// until the navigation finishes.  The navigation is done by assigning
// location.href in the frame |adapter|. Returns true if the page was loaded
// successfully and the last committed URL matches |url|.
WARN_UNUSED_RESULT bool NavigateToURLFromRenderer(
    const ToRenderFrameHost& adapter,
    const GURL& url);

void LoadDataWithBaseURL(Shell* window,
                         const GURL& url,
                         const std::string& data,
                         const GURL& base_url);

// Navigates |window| to |url|, blocking until the given number of navigations
// finishes.
void NavigateToURLBlockUntilNavigationsComplete(Shell* window,
                                                const GURL& url,
                                                int number_of_navigations);

// Navigates |window| to |url|, blocks until the navigation finishes, and
// checks that the navigation did not commit (e.g., due to a crash or
// download).
bool NavigateToURLAndExpectNoCommit(Shell* window, const GURL& url);

// Reloads |window|, blocking until the given number of navigations finishes.
void ReloadBlockUntilNavigationsComplete(Shell* window,
                                         int number_of_navigations);

// Reloads |window| with bypassing cache flag, and blocks until the given number
// of navigations finishes.
void ReloadBypassingCacheBlockUntilNavigationsComplete(
    Shell* window,
    int number_of_navigations);

// Wait until an application modal dialog is requested.
void WaitForAppModalDialog(Shell* window);

// Extends the ToRenderFrameHost mechanism to content::Shells.
RenderFrameHost* ConvertToRenderFrameHost(Shell* shell);

// Writes an entry with the name and id of the first camera to the logs or
// an entry indicating that no camera is available. This must be invoked from
// the test method body, because at the time of invocation of
// testing::Test::SetUp() the BrowserMainLoop does not yet exist.
void LookupAndLogNameAndIdOfFirstCamera();

// Used to wait for a new Shell window to be created. Instantiate this object
// before the operation that will create the window.
class ShellAddedObserver {
 public:
  ShellAddedObserver();
  ~ShellAddedObserver();

  // Will run a message loop to wait for the new window if it hasn't been
  // created since the constructor.
  Shell* GetShell();

 private:
  void ShellCreated(Shell* shell);

  Shell* shell_;
  scoped_refptr<MessageLoopRunner> runner_;

  DISALLOW_COPY_AND_ASSIGN(ShellAddedObserver);
};

#if defined OS_MACOSX
// An observer of the RenderWidgetHostViewCocoa which is the NSView
// corresponding to the page.
class RenderWidgetHostViewCocoaObserver {
 public:
  // The method name for 'didAddSubview'.
  static constexpr char kDidAddSubview[] = "didAddSubview:";
  static constexpr char kShowDefinitionForAttributedString[] =
      "showDefinitionForAttributedString:atPoint:";

  // Returns the method swizzler for the given |method_name|. This is useful
  // when the original implementation of the method is needed.
  static base::mac::ScopedObjCClassSwizzler* GetSwizzler(
      const std::string& method_name);

  // Returns the unique RenderWidgetHostViewCocoaObserver instance (if any) for
  // the given WebContents. There can be at most one observer per WebContents
  // and to create a new observer the older one has to be deleted first.
  static RenderWidgetHostViewCocoaObserver* GetObserver(
      WebContents* web_contents);

  explicit RenderWidgetHostViewCocoaObserver(WebContents* web_contents);
  virtual ~RenderWidgetHostViewCocoaObserver();

  // Called when a new NSView is added as a subview of RWHVCocoa.
  // |rect_in_root_view| represents the bounds of the NSView in RWHVCocoa
  // coordinates. The view will be dismissed shortly after this call.
  virtual void DidAddSubviewWillBeDismissed(
      const gfx::Rect& rect_in_root_view) {}
  // Called when RenderWidgeHostViewCocoa is asked to show definition of
  // |for_word| using Mac's dictionary popup.
  virtual void OnShowDefinitionForAttributedString(
      const std::string& for_word) {}

  WebContents* web_contents() const { return web_contents_; }

 private:
  static void SetUpSwizzlers();

  static std::map<std::string,
                  std::unique_ptr<base::mac::ScopedObjCClassSwizzler>>
      rwhvcocoa_swizzlers_;
  static std::map<WebContents*, RenderWidgetHostViewCocoaObserver*> observers_;

  WebContents* const web_contents_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewCocoaObserver);
};

void SetWindowBounds(gfx::NativeWindow window, const gfx::Rect& bounds);

// This method will request the string (word) at |point| inside the |rwh| where
// |point| is with respect to the |rwh| coordinates. |result_callback| is called
// with the word as well as |baselinePoint| when the result comes back from the
// renderer. The baseline point is the position of the pop-up in AppKit
// coordinate system (inverted y-axis).
void GetStringAtPointForRenderWidget(
    RenderWidgetHost* rwh,
    const gfx::Point& point,
    base::Callback<void(const std::string&, const gfx::Point&)>
        result_callback);

// This method will request the string identified by |range| inside the |rwh|.
// When the result comes back, |result_callback| is invoked with the given text
// and its position in AppKit coordinates (inverted-y axis).
void GetStringFromRangeForRenderWidget(
    RenderWidgetHost* rwh,
    const gfx::Range& range,
    base::Callback<void(const std::string&, const gfx::Point&)>
        result_callback);

#endif

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_CONTENT_BROWSER_TEST_UTILS_H_
