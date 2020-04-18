// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"

#import "base/ios/block_types.h"
#import "base/logging.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Enum class describing the current coordinator and consumer state.
enum class ActivationState {
  DEACTIVATED,  // The coordinator has been stopped or has never been started.
                // its UIViewController has been fully dismissed.
  ACTIVATING,   // The coordinator has been started, but its UIViewController
                // hasn't finished being presented.
  ACTIVATED,    // The coordinator has been started and its UIViewController has
                // finished being presented.
  DEACTIVATING,  // The coordinator has been stopped, but its UIViewController
                 // hasn't finished being dismissed.
};
// Returns the presentation state to use after |current_state|.
ActivationState GetNextActivationState(ActivationState current_state) {
  switch (current_state) {
    case ActivationState::DEACTIVATED:
      return ActivationState::ACTIVATING;
    case ActivationState::ACTIVATING:
      return ActivationState::ACTIVATED;
    case ActivationState::ACTIVATED:
      return ActivationState::DEACTIVATING;
    case ActivationState::DEACTIVATING:
      return ActivationState::DEACTIVATED;
  }
}
}

@interface BrowserCoordinator ()
// The coordinator's presentation state.
@property(nonatomic, assign) ActivationState activationState;
// Child coordinators owned by this object.
@property(nonatomic, strong)
    NSMutableSet<BrowserCoordinator*>* childCoordinators;
// Parent coordinator of this object, if any.
@property(nonatomic, readwrite, weak) BrowserCoordinator* parentCoordinator;

// Updates |activationState| to the next appropriate value after the in-
// progress transition animation finishes.  If there is no animation occurring,
// the state is updated immediately.
- (void)updateActivationStateAfterTransition;

@end

@implementation BrowserCoordinator
@synthesize browser = _browser;
@synthesize dispatcher = _dispatcher;
@synthesize activationState = _activationState;
@synthesize childCoordinators = _childCoordinators;
@synthesize parentCoordinator = _parentCoordinator;

- (instancetype)init {
  if (self = [super init]) {
    _activationState = ActivationState::DEACTIVATED;
    _childCoordinators = [NSMutableSet set];
  }
  return self;
}

- (void)dealloc {
  for (BrowserCoordinator* child in self.children) {
    [self removeChildCoordinator:child];
  }
}

#pragma mark - Accessors

- (BOOL)isStarted {
  return self.activationState == ActivationState::ACTIVATING ||
         self.activationState == ActivationState::ACTIVATED;
}

- (void)setActivationState:(ActivationState)state {
  if (_activationState == state)
    return;
  DCHECK_EQ(state, GetNextActivationState(_activationState))
      << "Unexpected activation state.  Probably from calling |-stop| while"
      << "ACTIVATING or |-start| while DEACTIVATING.";
  _activationState = state;
  if (_activationState == ActivationState::DEACTIVATED) {
    [self viewControllerWasDeactivated];
  } else if (_activationState == ActivationState::ACTIVATED) {
    [self viewControllerWasActivated];
  }
}

#pragma mark - Public API

- (id)callableDispatcher {
  return self.dispatcher;
}

- (void)start {
  if (self.started)
    return;
  self.activationState = ActivationState::ACTIVATING;
  [self.parentCoordinator childCoordinatorDidStart:self];
  [self updateActivationStateAfterTransition];
}

- (void)stop {
  if (!self.started)
    return;
  [self.parentCoordinator childCoordinatorWillStop:self];
  self.activationState = ActivationState::DEACTIVATING;
  for (BrowserCoordinator* child in self.children) {
    [child stop];
  }
  [self updateActivationStateAfterTransition];
}

#pragma mark - Private

- (void)updateActivationStateAfterTransition {
  DCHECK(self.activationState == ActivationState::ACTIVATING ||
         self.activationState == ActivationState::DEACTIVATING);
  ActivationState nextState = GetNextActivationState(self.activationState);
  id<UIViewControllerTransitionCoordinator> transitionCoordinator =
      self.viewController.transitionCoordinator;
  if (transitionCoordinator) {
    __weak BrowserCoordinator* weakSelf = self;
    [transitionCoordinator animateAlongsideTransition:nil
                                           completion:^(id context) {
                                             weakSelf.activationState =
                                                 nextState;
                                           }];
  } else {
    self.activationState = nextState;
  }
}

@end

@implementation BrowserCoordinator (Internal)
// Concrete implementations must implement a |viewController| property.
@dynamic viewController;

- (NSSet*)children {
  return [self.childCoordinators copy];
}

- (void)addChildCoordinator:(BrowserCoordinator*)childCoordinator {
  CHECK([self respondsToSelector:@selector(viewController)])
      << "BrowserCoordinator implementations must provide a viewController "
         "property.";
  [self.childCoordinators addObject:childCoordinator];
  childCoordinator.parentCoordinator = self;
  childCoordinator.browser = self.browser;
  childCoordinator.dispatcher = self.dispatcher;
  [childCoordinator wasAddedToParentCoordinator:self];
}

- (void)removeChildCoordinator:(BrowserCoordinator*)childCoordinator {
  if (![self.childCoordinators containsObject:childCoordinator])
    return;
  // Remove the grand-children first.
  for (BrowserCoordinator* grandChild in childCoordinator.children) {
    [childCoordinator removeChildCoordinator:grandChild];
  }
  // Remove the child.
  [childCoordinator willBeRemovedFromParentCoordinator];
  [self.childCoordinators removeObject:childCoordinator];
  childCoordinator.parentCoordinator = nil;
  childCoordinator.browser = nil;
}

- (void)wasAddedToParentCoordinator:(BrowserCoordinator*)parentCoordinator {
  // Default implementation is a no-op.
}

- (void)willBeRemovedFromParentCoordinator {
  // Default implementation is a no-op.
}

- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator {
  // Default implementation is a no-op.
}

- (void)childCoordinatorWillStop:(BrowserCoordinator*)childCoordinator {
  // Default implementation is a no-op.
}

- (void)viewControllerWasActivated {
  // Default implementation is a no-op.
}

- (void)viewControllerWasDeactivated {
  // Default implementation is a no-op.
}

@end
