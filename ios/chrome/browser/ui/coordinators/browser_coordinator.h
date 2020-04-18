// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_H_

#import <UIKit/UIKit.h>

class Browser;
@class CommandDispatcher;

// Enum for defining the "mode" of the coordinator -- the way that its view
// controller is positioned in the view controller hierarchy. There's no
// property in the base class for this value, but subclasses may define a 'mode'
// property if they need to vary behavior or configuration based on the mode. In
// that case, the default should always be UNDEFINED.
typedef NS_ENUM(NSInteger, BrowserCoordinatorMode) {
  UNDEFINED = 0,
  PRESENTED,  // The view controller is presented.
  CONTAINED   // The view controller is contained.
};

// An object that manages a UI component via a view controller.
// This is the public interface to this class; subclasses should also import
// the Internal category header (browser_coordinator+internal.h). This header
// file declares all the methods and properties a subclass must either override,
// call, or reset.
@interface BrowserCoordinator : NSObject

// The browser object used by this coordinator and passed into any child
// coordinators added to it. This is a weak pointer, and setting this property
// doesn't transfer ownership of the browser.
@property(nonatomic, assign) Browser* browser;

// The dispatcher this object should use to register and send commands.
// By default this is populated with the parent coordinator's dispatcher.
@property(nonatomic, strong) CommandDispatcher* dispatcher;

// self.dispatcher cast to |id|. Subclasses should redefine this property
// to conform to whatever protocols its view controllers and mediators expect.
@property(nonatomic, readonly) id callableDispatcher;

// The basic lifecycle methods for coordinators are -start and -stop. These
// implementations notify the parent coordinator when this coordinator did start
// and will stop. Child classes are expected to override and call the superclass
// method at the end of -start and at the beginning of -stop.
// If the receiver is already started, -start is a no-op. If the receiver is
// already stopped or never started, -stop is a no-op. In those cases, the
// overriding implementations can early return withotu calling the superclass
// method:
//   SubCoordinator.mm:
//     - (void)start {
//       if (self.started) return;
//       ...
//       [super start];
//     }

// Starts the user interaction managed by the receiver. Typical implementations
// will create a view controller and then use |baseViewController| to present
// it. This method needs to be called at the end of the overriding
// implementation.
// Starting a started coordinator is a no-op in this implementation.
- (void)start NS_REQUIRES_SUPER;

// Stops the user interaction managed by the receiver. This method needs to be
// called at the beginning of the overriding implementation.
// Calling stop on a coordinator transitively calls stop on its children.
// Stopping a non-started or stopped coordinator is a no-op in this
// implementation.
- (void)stop NS_REQUIRES_SUPER;

@end

#endif  // IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_H_
