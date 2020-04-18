// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/synthetic_gesture_target_mac.h"

#include "base/mac/scoped_nsautorelease_pool.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#import "content/browser/renderer_host/render_widget_host_view_cocoa.h"

// Unlike some event APIs, Apple does not provide a way to programmatically
// build a zoom event. To work around this, we leverage ObjectiveC's flexible
// typing and build up an object with the right interface to provide a zoom
// event.
@interface SyntheticPinchEvent : NSObject

// Populated based on desired zoom level.
@property CGFloat magnification;
@property NSPoint locationInWindow;
@property NSEventType type;
@property NSEventPhase phase;

// Filled with default values.
@property(readonly) CGFloat deltaX;
@property(readonly) CGFloat deltaY;
@property(readonly) NSEventModifierFlags modifierFlags;
@property(readonly) NSTimeInterval timestamp;

@end

@implementation SyntheticPinchEvent

@synthesize magnification = magnification_;
@synthesize locationInWindow = locationInWindow_;
@synthesize type = type_;
@synthesize phase = phase_;
@synthesize deltaX = deltaX_;
@synthesize deltaY = deltaY_;
@synthesize modifierFlags = modifierFlags_;
@synthesize timestamp = timestamp_;

- (id)initWithMagnification:(float)magnification
           locationInWindow:(NSPoint)location {
  self = [super init];
  if (self) {
    type_ = NSEventTypeMagnify;
    phase_ = NSEventPhaseChanged;
    magnification_ = magnification;
    locationInWindow_ = location;

    deltaX_ = 0;
    deltaY_ = 0;
    modifierFlags_ = 0;

    // Default timestamp to current time.
    timestamp_ = [[NSDate date] timeIntervalSince1970];
  }

  return self;
}

+ (id)eventWithMagnification:(float)magnification
            locationInWindow:(NSPoint)location
                       phase:(NSEventPhase)phase {
  SyntheticPinchEvent* event =
      [[SyntheticPinchEvent alloc] initWithMagnification:magnification
                                        locationInWindow:location];
  event.phase = phase;
  return [event autorelease];
}

@end

using blink::WebInputEvent;
using blink::WebGestureEvent;

namespace content {

SyntheticGestureTargetMac::SyntheticGestureTargetMac(
    RenderWidgetHostImpl* host,
    RenderWidgetHostViewCocoa* cocoa_view)
    : SyntheticGestureTargetBase(host), cocoa_view_(cocoa_view) {}

void SyntheticGestureTargetMac::DispatchWebGestureEventToPlatform(
    const WebGestureEvent& web_gesture,
    const ui::LatencyInfo& latency_info) {
  // Create an autorelease pool so that we clean up any synthetic events we
  // generate.
  base::mac::ScopedNSAutoreleasePool pool;

  NSPoint content_local = NSMakePoint(
      web_gesture.PositionInWidget().x,
      [cocoa_view_ frame].size.height - web_gesture.PositionInWidget().y);
  NSPoint location_in_window =
      [cocoa_view_ convertPoint:content_local toView:nil];

  switch (web_gesture.GetType()) {
    case WebInputEvent::kGesturePinchBegin: {
      id cocoa_event =
          [SyntheticPinchEvent eventWithMagnification:0.0f
                                     locationInWindow:location_in_window
                                                phase:NSEventPhaseBegan];
      [cocoa_view_ handleBeginGestureWithEvent:cocoa_event
                       isSyntheticallyInjected:YES];
      return;
    }
    case WebInputEvent::kGesturePinchEnd: {
      id cocoa_event =
          [SyntheticPinchEvent eventWithMagnification:0.0f
                                     locationInWindow:location_in_window
                                                phase:NSEventPhaseEnded];
      [cocoa_view_ handleEndGestureWithEvent:cocoa_event];
      return;
    }
    case WebInputEvent::kGesturePinchUpdate: {
      id cocoa_event = [SyntheticPinchEvent
          eventWithMagnification:web_gesture.data.pinch_update.scale - 1.0f
                locationInWindow:location_in_window
                           phase:NSEventPhaseChanged];
      [cocoa_view_ magnifyWithEvent:cocoa_event];
      return;
    }
    default:
      NOTREACHED();
  }
}

void SyntheticGestureTargetMac::DispatchWebTouchEventToPlatform(
    const blink::WebTouchEvent& web_touch,
    const ui::LatencyInfo& latency_info) {
  render_widget_host()->GetView()->InjectTouchEvent(web_touch, latency_info);
}

}  // namespace content
