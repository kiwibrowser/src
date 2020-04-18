// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_INTERNAL_H_
#define IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_INTERNAL_H_

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"

// Internal API for subclasses and categories of BrowserCoordinator.
@interface BrowserCoordinator (Internal)

// Managed view controller of this object. Subclasses must define a
// property named |viewController| that has the specific UIViewController
// subclass they use; the subclass API will be able to access that view
// controller through this property.
@property(nonatomic, readonly) UIViewController* viewController;

// The child coordinators of this coordinator. To add or remove from this set,
// use the -addChildCoordinator: and -removeChildCoordinator: methods.
@property(nonatomic, readonly) NSSet<BrowserCoordinator*>* children;

// The coordinator that added this coordinator as a child, if any.
@property(nonatomic, readonly) BrowserCoordinator* parentCoordinator;

// YES if the receiver has been started; NO (the default) otherwise. Stopping
// the receiver resets this property to NO.
@property(nonatomic, readonly, getter=isStarted) BOOL started;

// Adds |coordinator| as a child, taking ownership of it, setting the receiver's
// viewController (if any) as the child's baseViewController, and setting
// the receiver's |browser| as the child's |browser|.
- (void)addChildCoordinator:(BrowserCoordinator*)childCoordinator;

// Removes |coordinator| as a child, relinquishing ownership of it. If
// |coordinator| isn't a child of the receiver, this method does nothing.
- (void)removeChildCoordinator:(BrowserCoordinator*)childCoordinator;

// Called when this coordinator is added to a parent coordinator.
- (void)wasAddedToParentCoordinator:(BrowserCoordinator*)parentCoordinator;

// Called when this coordinator is going to be removed from its parent
// coordinator.
- (void)willBeRemovedFromParentCoordinator;

// Called when this coordinator's UIViewController has finished being presented.
- (void)viewControllerWasActivated;

// Called when this coordinator's UIViewController has finished being dismissed.
- (void)viewControllerWasDeactivated;

// Called when a child coordinator did start. This is a blank template method.
// Subclasses can override this method when they need to know when their
// children start.
- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator;

// Called when a child coordinator will stop. This is a blank template method.
// Subclasses can override this method when they need to know when their
// children start.
- (void)childCoordinatorWillStop:(BrowserCoordinator*)childCoordinator;

@end

#endif  // IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_INTERNAL_H_
