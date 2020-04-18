// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"

#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TestCoordinator : BrowserCoordinator
@property(nonatomic) UIViewController* viewController;
@property(nonatomic, copy) void (^stopHandler)();
@property(nonatomic) BOOL wasAddedCalled;
@property(nonatomic) BOOL willBeRemovedCalled;
@property(nonatomic, copy) void (^willBeRemovedHandler)();
@property(nonatomic) BOOL removeCalled;
@property(nonatomic) BOOL childDidStartCalled;
@property(nonatomic) BOOL childWillStopCalled;
@end

@implementation TestCoordinator
@synthesize viewController = _viewController;
@synthesize stopHandler = _stopHandler;
@synthesize wasAddedCalled = _wasAddedCalled;
@synthesize willBeRemovedCalled = _willBeRemovedCalled;
@synthesize willBeRemovedHandler = _willBeRemovedHandler;
@synthesize removeCalled = _removeCalled;
@synthesize childDidStartCalled = _childDidStartCalled;
@synthesize childWillStopCalled = _childWillStopCalled;

- (instancetype)init {
  if (!(self = [super init]))
    return nil;

  _viewController = [[UIViewController alloc] init];
  return self;
}

- (void)stop {
  [super stop];
  if (self.stopHandler)
    self.stopHandler();
}

- (void)wasAddedToParentCoordinator:(BrowserCoordinator*)parentCoordinator {
  [super wasAddedToParentCoordinator:parentCoordinator];
  self.wasAddedCalled = YES;
}

- (void)willBeRemovedFromParentCoordinator {
  [super willBeRemovedFromParentCoordinator];
  self.willBeRemovedCalled = YES;
  if (self.willBeRemovedHandler)
    self.willBeRemovedHandler();
}

- (void)removeChildCoordinator:(BrowserCoordinator*)childCoordinator {
  [super removeChildCoordinator:childCoordinator];
  self.removeCalled = YES;
}

- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator {
  [super childCoordinatorDidStart:childCoordinator];
  self.childDidStartCalled = YES;
}

- (void)childCoordinatorWillStop:(BrowserCoordinator*)childCoordinator {
  [super childCoordinatorWillStop:childCoordinator];
  self.childWillStopCalled = YES;
}

@end

@interface NonOverlayableCoordinator : TestCoordinator
@end

@implementation NonOverlayableCoordinator

- (BOOL)canAddOverlayCoordinator:(BrowserCoordinator*)overlayCoordinator {
  return NO;
}

@end

// Tests that -stop isn't called when a BrowserCoordinator is destroyed.
TEST_F(BrowserCoordinatorTest, TestDontStopOnDealloc) {
  __block BOOL called = NO;

  {
    TestCoordinator* coordinator = [[TestCoordinator alloc] init];
    coordinator.stopHandler = ^{
      called = YES;
    };
  }

  EXPECT_FALSE(called);
}

// Test that parents know who their children are, and that children know who
// their parent is.
TEST_F(BrowserCoordinatorTest, TestChildren) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  TestCoordinator* child = [[TestCoordinator alloc] init];

  [parent addChildCoordinator:child];
  EXPECT_TRUE([parent.children containsObject:child]);
  EXPECT_EQ(parent, child.parentCoordinator);

  [parent removeChildCoordinator:child];
  EXPECT_FALSE([parent.children containsObject:child]);
  EXPECT_EQ(nil, child.parentCoordinator);

  TestCoordinator* otherParent = [[TestCoordinator alloc] init];
  TestCoordinator* otherChild = [[TestCoordinator alloc] init];
  [otherParent addChildCoordinator:otherChild];

  // -removeChildCoordinator of a non-child should have no affect.
  [parent removeChildCoordinator:otherChild];
  EXPECT_TRUE([otherParent.children containsObject:otherChild]);
  EXPECT_EQ(otherParent, otherChild.parentCoordinator);
}

TEST_F(BrowserCoordinatorTest, AddedRemoved) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  TestCoordinator* child = [[TestCoordinator alloc] init];

  // Add to the parent.
  EXPECT_FALSE(child.wasAddedCalled);
  EXPECT_FALSE(child.willBeRemovedCalled);
  [parent addChildCoordinator:child];
  EXPECT_TRUE(child.wasAddedCalled);
  EXPECT_FALSE(child.willBeRemovedCalled);

  // Remove from the parent.
  [parent removeChildCoordinator:child];
  EXPECT_TRUE(child.willBeRemovedCalled);
}

// Tests that the didState/willStop methods are called.
TEST_F(BrowserCoordinatorTest, DidStartWillStop) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  TestCoordinator* child = [[TestCoordinator alloc] init];
  [parent addChildCoordinator:child];
  EXPECT_FALSE(parent.childDidStartCalled);
  EXPECT_FALSE(parent.childWillStopCalled);

  [child start];
  EXPECT_TRUE(parent.childDidStartCalled);
  EXPECT_FALSE(parent.childWillStopCalled);

  [child stop];
  EXPECT_TRUE(parent.childDidStartCalled);
  EXPECT_TRUE(parent.childWillStopCalled);
}

// Tests that calling -stop on a coordinator also recursively stops any children
// of that coordinator that have been started.
TEST_F(BrowserCoordinatorTest, StopStopsStartedChildren) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  TestCoordinator* child = [[TestCoordinator alloc] init];
  [parent addChildCoordinator:child];
  [parent start];
  [child start];
  __block BOOL called = NO;
  child.stopHandler = ^{
    called = YES;
  };
  EXPECT_FALSE(called);

  // Call stop on the parent.
  [parent stop];

  // It should have called stop on the child.
  EXPECT_TRUE(called);
}

// Tests that calling -stop on a coordinator does *not* call -stop on children
// that haven't been started.
TEST_F(BrowserCoordinatorTest, StopStopsNonStartedChildren) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  TestCoordinator* child = [[TestCoordinator alloc] init];
  [parent addChildCoordinator:child];
  [parent start];
  __block BOOL called = NO;
  child.stopHandler = ^{
    called = YES;
  };
  EXPECT_FALSE(called);

  // Call stop on the parent.
  [parent stop];

  // It should not have called stop on the child.
  EXPECT_TRUE(called);
}

// Tests that removing a child also sets the child's browser to nil, even if
// the child itself isn't nil.
TEST_F(BrowserCoordinatorTest, BrowserIsNilAfterCoordinatorIsRemoved) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  TestCoordinator* child = [[TestCoordinator alloc] init];
  parent.browser = GetBrowser();
  [parent addChildCoordinator:child];

  EXPECT_NE(nil, child.browser);

  // Remove the child.
  [parent removeChildCoordinator:child];

  EXPECT_EQ(nil, child.browser);
}

// Tests that children are recursively removed.
TEST_F(BrowserCoordinatorTest, RemoveRemovesGrandChildren) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  TestCoordinator* child = [[TestCoordinator alloc] init];
  TestCoordinator* grandChild = [[TestCoordinator alloc] init];
  [child addChildCoordinator:grandChild];
  [parent addChildCoordinator:child];

  EXPECT_FALSE(grandChild.willBeRemovedCalled);
  EXPECT_FALSE(child.removeCalled);

  // Remove the child.
  [parent removeChildCoordinator:child];

  EXPECT_TRUE(grandChild.willBeRemovedCalled);
  EXPECT_TRUE(child.removeCalled);
}

// Tests that grandchildren are removed before -willRemove is called on the
// child.
TEST_F(BrowserCoordinatorTest,
       RemoveRemovesGrandChildThenCallWillRemoveOnChild) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  TestCoordinator* child = [[TestCoordinator alloc] init];
  TestCoordinator* grandChild = [[TestCoordinator alloc] init];
  [child addChildCoordinator:grandChild];
  [parent addChildCoordinator:child];
  EXPECT_FALSE(grandChild.willBeRemovedCalled);
  EXPECT_FALSE(child.removeCalled);
  __weak TestCoordinator* weakChild = child;
  child.willBeRemovedHandler = ^{
    EXPECT_TRUE(grandChild.willBeRemovedCalled);
    EXPECT_TRUE(weakChild.removeCalled);
  };

  // Remove the child.
  [parent removeChildCoordinator:child];

  EXPECT_TRUE(child.willBeRemovedCalled);
}

// Tests that multiple grandchildren are removed.
TEST_F(BrowserCoordinatorTest, RemoveChildWithMultipleGrandChildren) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  TestCoordinator* child = [[TestCoordinator alloc] init];
  TestCoordinator* grandChild1 = [[TestCoordinator alloc] init];
  TestCoordinator* grandChild2 = [[TestCoordinator alloc] init];
  [child addChildCoordinator:grandChild1];
  [child addChildCoordinator:grandChild2];
  [parent addChildCoordinator:child];

  // Remove the child.
  [parent removeChildCoordinator:child];
}

// Tests that children inherit the dispatcher of the parent when it's nil.
TEST_F(BrowserCoordinatorTest, NilDispatcherInherited) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  EXPECT_EQ(nil, parent.dispatcher);
  TestCoordinator* child = [[TestCoordinator alloc] init];
  EXPECT_EQ(nil, child.dispatcher);
  [parent addChildCoordinator:child];
  EXPECT_EQ(nil, child.dispatcher);
}

// Tests that children inherit the dispatcher of the parent when it's nonnil.
TEST_F(BrowserCoordinatorTest, DispatcherInherited) {
  TestCoordinator* parent = [[TestCoordinator alloc] init];
  EXPECT_EQ(nil, parent.dispatcher);
  TestCoordinator* child = [[TestCoordinator alloc] init];
  EXPECT_EQ(nil, child.dispatcher);
  parent.dispatcher = [[CommandDispatcher alloc] init];
  [parent addChildCoordinator:child];
  EXPECT_EQ(parent.dispatcher, child.dispatcher);
}
