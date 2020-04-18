// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/browser_action_button.h"

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/app_menu/app_menu_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_container_view.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"
#import "chrome/browser/ui/cocoa/test/run_loop_testing.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/global_error/global_error.h"
#include "chrome/browser/ui/global_error/global_error_service.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "chrome/browser/ui/toolbar/component_toolbar_actions_factory.h"
#include "chrome/browser/ui/toolbar/media_router_action.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/views/scoped_macviews_browser_mode.h"
#include "extensions/common/extension_builder.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/events/test/cocoa_test_event_utils.h"

// A helper class to wait for a menu to open and close.
@interface MenuWatcher : NSObject
- (id)initWithController:(MenuControllerCocoa*)controller;
@property(nonatomic, assign) base::Closure openClosure;
@property(nonatomic, assign) base::Closure closeClosure;
@end

namespace {

using ActionType = extensions::ExtensionBuilder::ActionType;

const int kMenuPadding = 26;

// A simple error class that has a menu item.
class MenuError : public GlobalError {
 public:
  MenuError() {}
  ~MenuError() override {}

  bool HasMenuItem() override { return true; }
  int MenuItemCommandID() override {
    // An arbitrary high id so that it's not taken.
    return 65536;
  }
  base::string16 MenuItemLabel() override {
    const char kErrorMessage[] =
        "This is a really long error message that will cause the app menu "
        "to have increased width";
    return base::ASCIIToUTF16(kErrorMessage);
  }
  void ExecuteMenuItem(Browser* browser) override {}

  bool HasBubbleView() override { return false; }
  bool HasShownBubbleView() override { return false; }
  void ShowBubbleView(Browser* browser) override {}
  GlobalErrorBubbleViewBase* GetBubbleView() override { return nullptr; }

 private:
  DISALLOW_COPY_AND_ASSIGN(MenuError);
};

// Returns the center point for a particular |view|.
NSPoint GetCenterPoint(NSView* view) {
  NSWindow* window = [view window];
  NSScreen* screen = [window screen];
  DCHECK(screen);

  // Converts the center position of the view into the coordinates accepted
  // by ui_controls methods.
  NSRect bounds = [view bounds];
  NSPoint center = NSMakePoint(NSMidX(bounds), NSMidY(bounds));
  center = [view convertPoint:center toView:nil];
  center = ui::ConvertPointFromWindowToScreen(window, center);
  return NSMakePoint(center.x, [screen frame].size.height - center.y);
}

// Moves the mouse (synchronously) to the center of the given |view|.
void MoveMouseToCenter(NSView* view) {
  NSPoint centerPoint = GetCenterPoint(view);
  base::RunLoop runLoop;
  ui_controls::SendMouseMoveNotifyWhenDone(
      centerPoint.x, centerPoint.y, runLoop.QuitClosure());
  runLoop.Run();
}

// Simulates a right-click on the action button in the overflow menu.
void DispatchClickOnOverflowedAction(BrowserActionButton* action_button) {
  NSEvent* event = nil;  // The event doesn't matter when sending directly.
  [action_button rightMouseDown:event];
}

// ui_controls:: methods don't play nice when there is an open menu (like the
// app menu). Instead, ClickOnOverflowedAction() simulates a right click by
// feeding the event directly to the button. In regular interaction, this is
// dispatched after menuDidClose:. To simulate that, start the menu closing
// (with no action), but invoke the action on the button directly when the close
// is observed.
void ClickOnOverflowedAction(
    base::scoped_nsobject<MenuWatcher> app_menu_watcher,
    AppMenuController* app_menu_controller) {
  // The app menu should start as open (since that's where the overflowed
  // actions are).
  EXPECT_TRUE([app_menu_controller isMenuOpen]);

  BrowserActionsController* overflow_controller =
      [app_menu_controller browserActionsController];

  ASSERT_TRUE(overflow_controller);
  BrowserActionButton* action_button = [overflow_controller buttonWithIndex:0];

  // The action should be attached to a superview.
  EXPECT_TRUE([action_button superview]);

  base::Closure invoke_action = base::Bind(&DispatchClickOnOverflowedAction,
                                           base::Unretained(action_button));
  [app_menu_watcher setCloseClosure:invoke_action];

  // Close the app menu.
  [app_menu_controller cancel];
}

}  // namespace

// A simple helper menu delegate that will keep track of if a menu is opened,
// and closes them immediately (which is useful because message loops with
// menus open in Cocoa don't play nicely with testing).
@interface MenuHelper : NSObject<NSMenuDelegate> {
  // Whether or not a menu has been opened. This can be reset so the helper can
  // be used multiple times.
  BOOL menuOpened_;

  // The closure to run when the menu opens, if any.
  base::Closure openClosure_;

  // A function to be called to verify state while the menu is open.
  base::Closure verify_;
}

@property(nonatomic, assign) BOOL menuOpened;
@property(nonatomic, assign) base::Closure openClosure;
@property(nonatomic, assign) base::Closure verify;

@end

@implementation MenuHelper

- (void)menuWillOpen:(NSMenu*)menu {
  menuOpened_ = YES;
  if (!openClosure_.is_null())
    openClosure_.Run();
  if (!verify_.is_null())
    verify_.Run();
  [menu cancelTracking];
}

@synthesize menuOpened = menuOpened_;
@synthesize openClosure = openClosure_;
@synthesize verify = verify_;

@end

@implementation MenuWatcher {
  // The MenuControllerCocoa for the menu this object is watching.
  MenuControllerCocoa* menuController_;

  // The closure to run when the menu opens, if any.
  base::Closure openClosure_;

  // The closure to run when the menu closes, if any.
  base::Closure closeClosure_;
}

@synthesize openClosure = openClosure_;
@synthesize closeClosure = closeClosure_;

- (id)initWithController:(MenuControllerCocoa*)controller {
  if (self = [super init]) {
    menuController_ = controller;
    [[NSNotificationCenter defaultCenter]
        addObserver:self
         selector:@selector(menuDidOpen:)
       name:kMenuControllerMenuWillOpenNotification
        object:menuController_];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
         selector:@selector(menuDidClose:)
       name:kMenuControllerMenuDidCloseNotification
        object:menuController_];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)menuDidClose:(NSNotification*)notification {
  if (!closeClosure_.is_null()) {
    // Run |closeClosure_| synchronously since it may depend on objects that are
    // torn down once the menu is closed and execution has returned to the main
    // run loop.
    closeClosure_.Run();
    closeClosure_.Reset();
  }
}

- (void)menuDidOpen:(NSNotification*)notification {
  if (!openClosure_.is_null()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::ResetAndReturn(&openClosure_));
  }
}

@end

class BrowserActionButtonUiTest : public extensions::ExtensionBrowserTest {
 protected:
  BrowserActionButtonUiTest() {}
  ~BrowserActionButtonUiTest() override {}

  void SetUpOnMainThread() override {
    extensions::ExtensionBrowserTest::SetUpOnMainThread();
    toolbarController_ =
        [[BrowserWindowController
            browserWindowControllerForWindow:browser()->
                window()->GetNativeWindow()]
            toolbarController];
    ASSERT_TRUE(toolbarController_);
    appMenuController_ = [toolbarController_ appMenuController];
    model_ = ToolbarActionsModel::Get(profile());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    extensions::ExtensionBrowserTest::SetUpCommandLine(command_line);
    ToolbarActionsBar::disable_animations_for_testing_ = true;
  }

  void TearDownOnMainThread() override {
    ToolbarActionsBar::disable_animations_for_testing_ = false;
    extensions::ExtensionBrowserTest::TearDownOnMainThread();
  }

  // Opens the app menu and the context menu of the overflowed action, and
  // checks that the menus get opened/closed properly.
  void OpenAppMenuAndActionContextMenu(MenuHelper* context_menu_helper) {
    // Move the mouse over the app menu button.
    MoveMouseToCenter(appMenuButton());

    // No menu yet (on the browser action).
    EXPECT_FALSE([context_menu_helper menuOpened]);
    base::RunLoop run_loop;
    base::scoped_nsobject<MenuWatcher> app_menu_watcher(
        [[MenuWatcher alloc] initWithController:appMenuController()]);

    base::Closure close_with_action =
        base::Bind(&ClickOnOverflowedAction, app_menu_watcher,
                   base::Unretained(appMenuController()));
    [app_menu_watcher setOpenClosure:close_with_action];

    // Quit the RunLoop below when the context menu opens. This dictates that
    // the MenuHelper's verify action has also run.
    [context_menu_helper setOpenClosure:run_loop.QuitClosure()];

    // Show the app menu.
    ui_controls::SendMouseEvents(ui_controls::LEFT,
                                 ui_controls::DOWN | ui_controls::UP);
    run_loop.Run();

    // The menu opened on the main bar's action button rather than the
    // overflow's since Cocoa does not support running a menu within a menu.
    EXPECT_TRUE([context_menu_helper menuOpened]);
  }

  ToolbarController* toolbarController() { return toolbarController_; }
  AppMenuController* appMenuController() { return appMenuController_; }
  ToolbarActionsModel* model() { return model_; }
  NSView* appMenuButton() { return [toolbarController_ appMenuButton]; }

 private:
  ToolbarController* toolbarController_ = nil;
  AppMenuController* appMenuController_ = nil;
  ToolbarActionsModel* model_ = nullptr;

  test::ScopedMacViewsBrowserMode cocoa_browser_mode_{false};

  DISALLOW_COPY_AND_ASSIGN(BrowserActionButtonUiTest);
};

// Verifies that the action is "popped out" of overflow; that is, it is visible
// on the main bar, and is set as the popped out action on the controlling
// ToolbarActionsBar.
void CheckActionIsPoppedOut(BrowserActionsController* actionsController,
                            BrowserActionButton* actionButton) {
  EXPECT_EQ([actionsController containerView], [actionButton superview]);
  EXPECT_EQ([actionButton viewController],
            [actionsController toolbarActionsBar]->popped_out_action());
  // Since the button is popped out for a popup or context menu, it should be
  // highlighted.
  EXPECT_TRUE([actionButton isHighlighted]);
}

// Test that opening a context menu works for both actions on the main bar and
// actions in the overflow menu.
IN_PROC_BROWSER_TEST_F(BrowserActionButtonUiTest,
                       ContextMenusOnMainAndOverflow) {
  // Add an extension with a browser action.
  scoped_refptr<const extensions::Extension> extension =
      extensions::ExtensionBuilder("browser_action")
          .SetAction(ActionType::BROWSER_ACTION)
          .Build();
  extension_service()->AddExtension(extension.get());
  ASSERT_EQ(1u, model()->toolbar_items().size());

  BrowserActionsController* actionsController =
      [toolbarController() browserActionsController];
  BrowserActionButton* actionButton = [actionsController buttonWithIndex:0];
  ASSERT_TRUE(actionButton);

  // Stub out the action button's normal context menu with a fake one so we
  // can track when it opens.
  base::scoped_nsobject<NSMenu> testContextMenu(
        [[NSMenu alloc] initWithTitle:@""]);
  base::scoped_nsobject<MenuHelper> menuHelper([[MenuHelper alloc] init]);
  [testContextMenu setDelegate:menuHelper.get()];
  [actionButton setTestContextMenu:testContextMenu.get()];

  // Right now, the action button should be visible (attached to a superview).
  EXPECT_TRUE([actionButton superview]);

  // Move the mouse to the center of the action button, preparing to click.
  MoveMouseToCenter(actionButton);

  {
    // No menu should yet be open.
    EXPECT_FALSE([menuHelper menuOpened]);
    base::RunLoop runLoop;
    ui_controls::SendMouseEventsNotifyWhenDone(
        ui_controls::RIGHT,
        ui_controls::DOWN | ui_controls::UP,
        runLoop.QuitClosure());
    runLoop.Run();
    // The menu should have opened from the click.
    EXPECT_TRUE([menuHelper menuOpened]);
    EXPECT_FALSE([actionButton isHighlighted]);
  }

  // Reset the menu helper so we can use it again.
  [menuHelper setMenuOpened:NO];
  [menuHelper setVerify:base::Bind(
      CheckActionIsPoppedOut, actionsController, actionButton)];

  // Shrink the visible count to be 0. This should hide the action button.
  model()->SetVisibleIconCount(0);
  EXPECT_EQ(nil, [actionButton superview]);

  OpenAppMenuAndActionContextMenu(menuHelper.get());
}

IN_PROC_BROWSER_TEST_F(BrowserActionButtonUiTest,
                       MediaRouterActionContextMenuInOverflow) {
  model()->AddComponentAction(
      ComponentToolbarActionsFactory::kMediaRouterActionId);
  ASSERT_EQ(1u, model()->toolbar_items().size());

  BrowserActionButton* actionButton =
      [[toolbarController() browserActionsController] buttonWithIndex:0];
  ASSERT_TRUE(actionButton);

  // Stub out the action button's normal context menu with a fake one so we
  // can track when it opens.
  base::scoped_nsobject<NSMenu> testContextMenu(
      [[NSMenu alloc] initWithTitle:@""]);
  base::scoped_nsobject<MenuHelper> menuHelper([[MenuHelper alloc] init]);
  [testContextMenu setDelegate:menuHelper.get()];
  [actionButton setTestContextMenu:testContextMenu.get()];

  model()->SetActionVisibility(
      ComponentToolbarActionsFactory::kMediaRouterActionId, false);

  OpenAppMenuAndActionContextMenu(menuHelper.get());

  ToolbarActionsBar* actionsBar =
      [[toolbarController() browserActionsController] toolbarActionsBar];
  // The action should be back in the overflow.
  EXPECT_FALSE(
      actionsBar->IsActionVisibleOnMainBar(actionsBar->GetActions()[0]));
}

// Checks the layout of the overflow bar in the app menu.
void CheckAppMenuLayout(ToolbarController* toolbarController,
                        int overflowStartIndex,
                        const std::string& error_message,
                        const base::Closure& closure) {
  AppMenuController* appMenuController =
      [toolbarController appMenuController];
  // The app menu should start as open (since that's where the overflowed
  // actions are).
  EXPECT_TRUE([appMenuController isMenuOpen]) << error_message;
  BrowserActionsController* overflowController =
      [appMenuController browserActionsController];
  ASSERT_TRUE(overflowController) << error_message;

  ToolbarActionsBar* overflowBar = [overflowController toolbarActionsBar];
  BrowserActionsContainerView* overflowContainer =
      [overflowController containerView];
  NSMenu* menu = [appMenuController menu];

  // The overflow container should be within the bounds of the app menu, as
  // should its parents.
  int menu_width = [menu size].width;
  NSRect frame = [overflowContainer frame];
  // The container itself should be indented in the menu.
  EXPECT_GT(NSMinX(frame), 0) << error_message;
  // Hierarchy: The overflow container is owned by two different views in the
  // app menu. Each superview should start at 0 in the x-axis.
  EXPECT_EQ(0, NSMinX([[overflowContainer superview] frame])) << error_message;
  EXPECT_EQ(0, NSMinX([[[overflowContainer superview] superview] frame])) <<
      error_message;
  // The overflow container should fully fit in the app menu, including the
  // space taken away for padding, and should have its desired size.
  EXPECT_LE(NSWidth(frame), menu_width - kMenuPadding) << error_message;
  EXPECT_EQ(NSWidth(frame), overflowBar->GetFullSize().width())
      << error_message;

  // Every button that has an index lower than the overflow start index (i.e.,
  // every button on the main toolbar) should not be attached to a view.
  for (int i = 0; i < overflowStartIndex; ++i) {
    BrowserActionButton* button = [overflowController buttonWithIndex:i];
    EXPECT_FALSE([button superview]) << error_message;
    EXPECT_GE(0, NSMaxX([button frame])) << error_message;
  }

  // Every other button should be attached to a view, and should be at the
  // proper bounds. Calculating each button's proper bounds here would just be
  // a duplication of the logic in the method, but we can test that each button
  // a) is within the overflow container's bounds, and
  // b) doesn't intersect with another button.
  // If both of those are true, then we're probably good.
  for (NSUInteger i = overflowStartIndex;
       i < [overflowController buttonCount]; ++i) {
    BrowserActionButton* button = [overflowController buttonWithIndex:i];
    EXPECT_TRUE([button superview]) << error_message;
    EXPECT_TRUE(NSContainsRect([overflowContainer bounds], [button frame])) <<
        error_message;
    for (NSUInteger j = 0; j < i; ++j) {
      EXPECT_FALSE(
          NSContainsRect([[overflowController buttonWithIndex:j] frame],
                         [button frame])) << error_message;
    }
  }

  // Close the app menu.
  [appMenuController cancel];
  EXPECT_FALSE([appMenuController isMenuOpen]) << error_message;
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, closure);
}

// Tests the layout of the overflow container in the app menu.
// Disabled due to crashing flakily, see crbug.com/602203.
IN_PROC_BROWSER_TEST_F(BrowserActionButtonUiTest,
                       DISABLED_TestOverflowContainerLayout) {
  // Add a bunch of extensions - enough to trigger multiple rows in the overflow
  // menu.
  const int kNumExtensions = 12;
  for (int i = 0; i < kNumExtensions; ++i) {
    scoped_refptr<const extensions::Extension> extension =
        extensions::ExtensionBuilder(base::StringPrintf("extension%d", i))
            .SetAction(ActionType::BROWSER_ACTION)
            .Build();
    extension_service()->AddExtension(extension.get());
  }
  ASSERT_EQ(kNumExtensions, static_cast<int>(model()->toolbar_items().size()));

  // A helper function to open the app menu and call the check function.
  auto resizeAndActivateAppMenu = [this](int visible_count,
                                         const std::string& error_message) {
    model()->SetVisibleIconCount(kNumExtensions - visible_count);
    MoveMouseToCenter(appMenuButton());

    {
      base::RunLoop runLoop;
      // Click on the app menu, and pass in a callback to continue the test in
      // CheckAppMenuLayout (due to the blocking nature of Cocoa menus,
      // passing in runLoop.QuitClosure() is not sufficient here.)
      ui_controls::SendMouseEventsNotifyWhenDone(
          ui_controls::LEFT, ui_controls::DOWN | ui_controls::UP,
          base::Bind(&CheckAppMenuLayout,
                     base::Unretained(toolbarController()),
                     kNumExtensions - visible_count,
                     error_message,
                     runLoop.QuitClosure()));
      runLoop.Run();
    }
  };

  // Test the layout with gradually more extensions hidden.
  for (int i = 1; i <= kNumExtensions; ++i)
    resizeAndActivateAppMenu(i, base::StringPrintf("Normal: %d", i));

  // Adding a global error adjusts the app menu size, and has been known to mess
  // up the overflow container's bounds (crbug.com/511326).
  GlobalErrorService* error_service =
      GlobalErrorServiceFactory::GetForProfile(profile());
  error_service->AddGlobalError(std::make_unique<MenuError>());

  // It's probably excessive to test every level of the overflow here. Test
  // having all actions overflowed, some actions overflowed, and one action
  // overflowed.
  resizeAndActivateAppMenu(kNumExtensions, "GlobalError Full");
  resizeAndActivateAppMenu(kNumExtensions / 2, "GlobalError Half");
  resizeAndActivateAppMenu(1, "GlobalError One");
}

void AddExtensionWithMenuOpen(ToolbarController* toolbarController,
                              ExtensionService* extensionService,
                              const base::Closure& closure) {
  AppMenuController* appMenuController =
      [toolbarController appMenuController];
  EXPECT_TRUE([appMenuController isMenuOpen]);

  scoped_refptr<const extensions::Extension> extension =
      extensions::ExtensionBuilder("extension")
          .SetAction(ActionType::BROWSER_ACTION)
          .Build();
  extensionService->AddExtension(extension.get());

  base::RunLoop().RunUntilIdle();

  // Close the app menu.
  [appMenuController cancel];
  EXPECT_FALSE([appMenuController isMenuOpen]);

  closure.Run();
}

// Test adding an extension while the app menu is open. Regression test for
// crbug.com/561237.
IN_PROC_BROWSER_TEST_F(BrowserActionButtonUiTest,
                       AddExtensionWithMenuOpen) {
  // Add an extension to ensure the overflow menu is present.
  scoped_refptr<const extensions::Extension> extension =
      extensions::ExtensionBuilder("original extension")
          .SetAction(ActionType::BROWSER_ACTION)
          .Build();
  extension_service()->AddExtension(extension.get());
  ASSERT_EQ(1, static_cast<int>(model()->toolbar_items().size()));
  model()->SetVisibleIconCount(0);

  MoveMouseToCenter(appMenuButton());

  base::RunLoop runLoop;
  // Click on the app menu, and pass in a callback to continue the test in
  // AddExtensionWithMenuOpen (due to the blocking nature of Cocoa menus,
  // passing in runLoop.QuitClosure() is not sufficient here.)
  base::scoped_nsobject<MenuWatcher> menuWatcher(
      [[MenuWatcher alloc] initWithController:appMenuController()]);
  [menuWatcher setOpenClosure:
      base::Bind(&AddExtensionWithMenuOpen,
                 base::Unretained(toolbarController()), extension_service(),
                 runLoop.QuitClosure())];
  ui_controls::SendMouseEvents(ui_controls::LEFT,
                               ui_controls::DOWN | ui_controls::UP);
  runLoop.Run();
}

// Test that activating an action that doesn't want to run on the page via the
// mouse and the keyboard works.
IN_PROC_BROWSER_TEST_F(BrowserActionButtonUiTest,
                       OpenMenuOnDisabledActionWithMouseOrKeyboard) {
  // Add an extension with a page action.
  scoped_refptr<const extensions::Extension> extension =
      extensions::ExtensionBuilder("page action")
          .SetAction(ActionType::PAGE_ACTION)
          .Build();
  extension_service()->AddExtension(extension.get());
  ASSERT_EQ(1u, model()->toolbar_items().size());

  BrowserActionsController* actionsController =
      [toolbarController() browserActionsController];
  BrowserActionButton* actionButton = [actionsController buttonWithIndex:0];
  ASSERT_TRUE(actionButton);

  // Stub out the action button's normal context menu with a fake one so we
  // can track when it opens.
  base::scoped_nsobject<NSMenu> testContextMenu(
      [[NSMenu alloc] initWithTitle:@""]);
  base::scoped_nsobject<MenuHelper> menuHelper([[MenuHelper alloc] init]);
  [testContextMenu setDelegate:menuHelper.get()];
  [actionButton setTestContextMenu:testContextMenu.get()];

  // The button should be attached.
  EXPECT_TRUE([actionButton superview]);

  // Move the mouse and click on the button. The menu should open.
  MoveMouseToCenter(actionButton);
  {
    EXPECT_FALSE([menuHelper menuOpened]);
    base::RunLoop runLoop;
    ui_controls::SendMouseEventsNotifyWhenDone(
        ui_controls::LEFT, ui_controls::DOWN | ui_controls::UP,
        runLoop.QuitClosure());
    runLoop.Run();
    EXPECT_TRUE([menuHelper menuOpened]);
  }

  // Reset the menu helper so we can use it again.
  [menuHelper setMenuOpened:NO];

  // Send the 'space' key to the button with it as the first responder. The menu
  // should open.
  [[actionButton window] makeFirstResponder:actionButton];
  EXPECT_TRUE([[actionButton window] firstResponder] == actionButton);
  {
    EXPECT_FALSE([menuHelper menuOpened]);
    base::RunLoop runLoop;
    ui_controls::SendKeyPressNotifyWhenDone([actionButton window],
                                            ui::VKEY_SPACE, false, false, false,
                                            false, runLoop.QuitClosure());
    runLoop.Run();
    EXPECT_TRUE([menuHelper menuOpened]);
  }
}

void CloseAppMenu(AppMenuController* appMenuController,
                  const base::Closure& quitClosure) {
  EXPECT_TRUE([appMenuController isMenuOpen]);
  [appMenuController cancel];
  quitClosure.Run();
}

// Tests opening the app menu with an overflow section needed, then closing it,
// removing the need for the overflow section, and re-opening it.
// Regression test for crbug.com/603241.
IN_PROC_BROWSER_TEST_F(BrowserActionButtonUiTest,
                       TestReopeningAppMenuWithOverflowNotNeeded) {
  // Add an extension with a browser action and overflow it in the menu.
  scoped_refptr<const extensions::Extension> extension =
      extensions::ExtensionBuilder("browser_action")
          .SetAction(ActionType::BROWSER_ACTION)
          .Build();
  extension_service()->AddExtension(extension.get());
  ASSERT_EQ(1u, model()->toolbar_items().size());
  model()->SetVisibleIconCount(0);

  // Move the mouse over the app menu button.
  MoveMouseToCenter(appMenuButton());

  auto openAndCloseAppMenu = [](AppMenuController* controller) {
    base::RunLoop runLoop;
    base::scoped_nsobject<MenuWatcher> menuWatcher(
        [[MenuWatcher alloc] initWithController:controller]);
    [menuWatcher setOpenClosure:
        base::Bind(&CloseAppMenu,
                   base::Unretained(controller),
                   runLoop.QuitClosure())];
    ui_controls::SendMouseEvents(ui_controls::LEFT,
                                 ui_controls::DOWN | ui_controls::UP);
    runLoop.Run();
  };
  openAndCloseAppMenu(appMenuController());

  // Move the extension back to the main bar, so an overflow bar is no longer
  // needed. Then open and close the app menu a couple times.
  // This tests that the menu properly cleans up after itself when an overflow
  // was present, and is no longer (fix for crbug.com/603241).
  model()->SetVisibleIconCount(1);
  openAndCloseAppMenu(appMenuController());
  openAndCloseAppMenu(appMenuController());
}
