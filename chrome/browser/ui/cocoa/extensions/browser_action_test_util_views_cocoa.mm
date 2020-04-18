// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/extensions/browser_action_test_util.h"

#include <stddef.h>

#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "build/buildflag.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/browser_window_cocoa.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/extensions/browser_action_button.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_container_view.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"
#import "chrome/browser/ui/cocoa/extensions/extension_popup_controller.h"
#include "chrome/browser/ui/cocoa/extensions/extension_popup_views_mac.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "chrome/browser/ui/views/toolbar/browser_action_test_util_views.h"
#include "chrome/browser/ui/views_mode_controller.h"
#include "chrome/common/chrome_constants.h"
#include "ui/base/theme_provider.h"
#include "ui/base/ui_features.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget.h"

namespace {

// The Cocoa implementation of the TestToolbarActionsBarHelper, which creates
// (and owns) a BrowserActionsController and BrowserActionsContainerView for
// testing purposes.
class TestToolbarActionsBarHelperCocoa : public TestToolbarActionsBarHelper {
 public:
  TestToolbarActionsBarHelperCocoa(Browser* browser,
                                   BrowserActionsController* mainController);
  ~TestToolbarActionsBarHelperCocoa() override;

  BrowserActionsController* controller() { return controller_.get(); }

 private:
  // The owned BrowserActionsContainerView and BrowserActionsController; the
  // mac implementation of the ToolbarActionsBar delegate and view.
  base::scoped_nsobject<BrowserActionsContainerView> containerView_;
  base::scoped_nsobject<BrowserActionsController> controller_;

  DISALLOW_COPY_AND_ASSIGN(TestToolbarActionsBarHelperCocoa);
};

TestToolbarActionsBarHelperCocoa::TestToolbarActionsBarHelperCocoa(
    Browser* browser,
    BrowserActionsController* mainController) {
  // Make sure that Cocoa has been bootstrapped.
  if (!base::mac::FrameworkBundle()) {
    // Look in the framework bundle for resources.
    base::FilePath path;
    base::PathService::Get(base::DIR_EXE, &path);
    path = path.Append(chrome::kFrameworkName);
    base::mac::SetOverrideFrameworkBundlePath(path);
  }

  containerView_.reset([[BrowserActionsContainerView alloc]
      initWithFrame:NSMakeRect(0, 0, 0, 15)]);
  controller_.reset(
      [[BrowserActionsController alloc] initWithBrowser:browser
                                          containerView:containerView_.get()
                                         mainController:mainController]);
}

TestToolbarActionsBarHelperCocoa::~TestToolbarActionsBarHelperCocoa() {
}

BrowserActionsController* GetController(Browser* browser,
                                        TestToolbarActionsBarHelper* helper) {
  if (helper) {
    return static_cast<TestToolbarActionsBarHelperCocoa*>(helper)->controller();
  }

  BrowserWindowCocoa* window =
      static_cast<BrowserWindowCocoa*>(browser->window());
  return [[window->cocoa_controller() toolbarController]
           browserActionsController];
}

BrowserActionButton* GetButton(
    Browser* browser,
    TestToolbarActionsBarHelper* helper,
    int index) {
  return [GetController(browser, helper) buttonWithIndex:index];
}

class ExtensionPopupTestManager {
 public:
  ExtensionPopupTestManager() = default;
  virtual ~ExtensionPopupTestManager() = default;

  virtual void DisableAnimations() = 0;
  virtual gfx::Size GetPopupSize(BrowserActionTestUtil* test_util) = 0;
  virtual void HidePopup(BrowserActionTestUtil* test_util) = 0;
  virtual gfx::Size GetMinPopupSize() = 0;
  virtual gfx::Size GetMaxPopupSize() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtensionPopupTestManager);
};

class ExtensionPopupTestManagerCocoa : public ExtensionPopupTestManager {
 public:
  ExtensionPopupTestManagerCocoa() = default;
  ~ExtensionPopupTestManagerCocoa() override = default;

  void DisableAnimations() override {
    [ExtensionPopupController setAnimationsEnabledForTesting:NO];
  }

  gfx::Size GetPopupSize(BrowserActionTestUtil* test_util) override {
    NSRect bounds = [[[ExtensionPopupController popup] view] bounds];
    return gfx::Size(NSSizeToCGSize(bounds.size));
  }

  void HidePopup(BrowserActionTestUtil* test_util) override {
    ExtensionPopupController* controller = [ExtensionPopupController popup];
    [controller close];
  }

  gfx::Size GetMinPopupSize() override {
    return gfx::Size(NSSizeToCGSize([ExtensionPopupController minPopupSize]));
  }

  gfx::Size GetMaxPopupSize() override {
    return gfx::Size(NSSizeToCGSize([ExtensionPopupController maxPopupSize]));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtensionPopupTestManagerCocoa);
};

class ExtensionPopupTestManagerViews : public ExtensionPopupTestManager {
 public:
  ExtensionPopupTestManagerViews() = default;

  ~ExtensionPopupTestManagerViews() override = default;

  void DisableAnimations() override {}

  gfx::Size GetPopupSize(BrowserActionTestUtil* test_util) override {
    gfx::NativeView popup = test_util->GetPopupNativeView();
    views::Widget* widget = views::Widget::GetWidgetForNativeView(popup);
    return widget->GetWindowBoundsInScreen().size();
  }

  void HidePopup(BrowserActionTestUtil* test_util) override {
    test_util->GetToolbarActionsBar()->HideActivePopup();
  }

  gfx::Size GetMinPopupSize() override {
    return gfx::Size(ExtensionPopupViewsMac::kMinWidth,
                     ExtensionPopupViewsMac::kMinHeight);
  }

  gfx::Size GetMaxPopupSize() override {
    return gfx::Size(ExtensionPopupViewsMac::kMaxWidth,
                     ExtensionPopupViewsMac::kMaxHeight);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtensionPopupTestManagerViews);
};

std::unique_ptr<ExtensionPopupTestManager> GetExtensionPopupTestManager() {
  if (!chrome::ShowAllDialogsWithViewsToolkit()) {
    return std::make_unique<ExtensionPopupTestManagerCocoa>();
  }
  return std::make_unique<ExtensionPopupTestManagerViews>();
}

}  // namespace

class BrowserActionTestUtilCocoa : public BrowserActionTestUtil {
 public:
  explicit BrowserActionTestUtilCocoa(Browser* browser);
  BrowserActionTestUtilCocoa(Browser* browser, bool is_real_window);

  ~BrowserActionTestUtilCocoa() override;

  // BrowserActionTestUtil:
  int NumberOfBrowserActions() override;
  int VisibleBrowserActions() override;
  void InspectPopup(int index) override;
  bool HasIcon(int index) override;
  gfx::Image GetIcon(int index) override;
  void Press(int index) override;
  std::string GetExtensionId(int index) override;
  std::string GetTooltip(int index) override;
  gfx::NativeView GetPopupNativeView() override;
  bool HasPopup() override;
  gfx::Size GetPopupSize() override;
  bool HidePopup() override;
  bool ActionButtonWantsToRun(size_t index) override;
  void SetWidth(int width) override;
  ToolbarActionsBar* GetToolbarActionsBar() override;
  std::unique_ptr<BrowserActionTestUtil> CreateOverflowBar() override;
  gfx::Size GetMinPopupSize() override;
  gfx::Size GetMaxPopupSize() override;
  bool CanBeResized() override;

 private:
  friend class BrowserActionTestUtil;

  BrowserActionTestUtilCocoa(Browser* browser,
                             BrowserActionTestUtilCocoa* main_bar);

  Browser* const browser_;  // weak

  // Our test helper, which constructs and owns the views if we don't have a
  // real browser window, or if this is an overflow version.
  std::unique_ptr<TestToolbarActionsBarHelper> test_helper_;
};

BrowserActionTestUtilCocoa::BrowserActionTestUtilCocoa(Browser* browser)
    : BrowserActionTestUtilCocoa(browser, true) {}

BrowserActionTestUtilCocoa::BrowserActionTestUtilCocoa(Browser* browser,
                                                       bool is_real_window)
    : browser_(browser) {
  if (!is_real_window)
    test_helper_.reset(new TestToolbarActionsBarHelperCocoa(browser, nullptr));
  // We disable animations on extension popups so that tests aren't waiting for
  // a popup to fade out.
  GetExtensionPopupTestManager()->DisableAnimations();
}

BrowserActionTestUtilCocoa::~BrowserActionTestUtilCocoa() {}

int BrowserActionTestUtilCocoa::NumberOfBrowserActions() {
  return [GetController(browser_, test_helper_.get()) buttonCount];
}

int BrowserActionTestUtilCocoa::VisibleBrowserActions() {
  return [GetController(browser_, test_helper_.get()) visibleButtonCount];
}

void BrowserActionTestUtilCocoa::InspectPopup(int index) {
  NOTREACHED();
}

bool BrowserActionTestUtilCocoa::HasIcon(int index) {
  return [GetButton(browser_, test_helper_.get(), index) image] != nil;
}

gfx::Image BrowserActionTestUtilCocoa::GetIcon(int index) {
  NSImage* ns_image = [GetButton(browser_, test_helper_.get(), index) image];
  // gfx::Image takes ownership of the |ns_image| reference. We have to increase
  // the ref count so |ns_image| stays around when the image object is
  // destroyed.
  base::mac::NSObjectRetain(ns_image);
  return gfx::Image(ns_image);
}

void BrowserActionTestUtilCocoa::Press(int index) {
  NSButton* button = GetButton(browser_, test_helper_.get(), index);
  [button performClick:nil];
}

std::string BrowserActionTestUtilCocoa::GetExtensionId(int index) {
  return
      [GetButton(browser_, test_helper_.get(), index) viewController]->GetId();
}

std::string BrowserActionTestUtilCocoa::GetTooltip(int index) {
  NSString* tooltip = [GetButton(browser_, test_helper_.get(), index) toolTip];
  return base::SysNSStringToUTF8(tooltip);
}

gfx::NativeView BrowserActionTestUtilCocoa::GetPopupNativeView() {
  ToolbarActionViewController* popup_owner =
      GetToolbarActionsBar()->popup_owner();
  return popup_owner ? popup_owner->GetPopupNativeView() : nil;
}

bool BrowserActionTestUtilCocoa::HasPopup() {
  return GetPopupNativeView() != nil;
}

gfx::Size BrowserActionTestUtilCocoa::GetPopupSize() {
  return GetExtensionPopupTestManager()->GetPopupSize(this);
}

bool BrowserActionTestUtilCocoa::HidePopup() {
  GetExtensionPopupTestManager()->HidePopup(this);
  return !HasPopup();
}

bool BrowserActionTestUtilCocoa::ActionButtonWantsToRun(size_t index) {
  return [GetButton(browser_, test_helper_.get(), index) wantsToRunForTesting];
}

void BrowserActionTestUtilCocoa::SetWidth(int width) {
  BrowserActionsContainerView* containerView =
      [GetController(browser_, test_helper_.get()) containerView];
  NSRect frame = [containerView frame];
  frame.size.width = width;
  [containerView setFrame:frame];
}

ToolbarActionsBar* BrowserActionTestUtilCocoa::GetToolbarActionsBar() {
  return [GetController(browser_, test_helper_.get()) toolbarActionsBar];
}

std::unique_ptr<BrowserActionTestUtil>
BrowserActionTestUtilCocoa::CreateOverflowBar() {
  CHECK(!GetToolbarActionsBar()->in_overflow_mode())
      << "Only a main bar can create an overflow bar!";
  return base::WrapUnique(new BrowserActionTestUtilCocoa(browser_, this));
}

gfx::Size BrowserActionTestUtilCocoa::GetMinPopupSize() {
  return GetExtensionPopupTestManager()->GetMinPopupSize();
}

gfx::Size BrowserActionTestUtilCocoa::GetMaxPopupSize() {
  return GetExtensionPopupTestManager()->GetMaxPopupSize();
}

bool BrowserActionTestUtilCocoa::CanBeResized() {
  BrowserActionsContainerView* containerView =
      [GetController(browser_, test_helper_.get()) containerView];
  return [containerView canBeResized];
}

BrowserActionTestUtilCocoa::BrowserActionTestUtilCocoa(
    Browser* browser,
    BrowserActionTestUtilCocoa* main_bar)
    : browser_(browser),
      test_helper_(new TestToolbarActionsBarHelperCocoa(
          browser_,
          GetController(browser_, main_bar->test_helper_.get()))) {}

// static
std::unique_ptr<BrowserActionTestUtil> BrowserActionTestUtil::Create(
    Browser* browser,
    bool is_real_window) {
#if BUILDFLAG(MAC_VIEWS_BROWSER)
  if (!views_mode_controller::IsViewsBrowserCocoa())
    return std::make_unique<BrowserActionTestUtilViews>(browser,
                                                        is_real_window);
#endif
  return std::make_unique<BrowserActionTestUtilCocoa>(browser, is_real_window);
}
