// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/accessibility_event_recorder.h"

#include <string>

#import <Cocoa/Cocoa.h>

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"

namespace content {

// Implementation of AccessibilityEventRecorder that uses AXObserver to
// watch for NSAccessibility events.
class AccessibilityEventRecorderMac : public AccessibilityEventRecorder {
 public:
  explicit AccessibilityEventRecorderMac(BrowserAccessibilityManager* manager,
                                         base::ProcessId pid);
  ~AccessibilityEventRecorderMac() override;

  // Callback executed every time we receive an event notification.
  void EventReceived(AXUIElementRef element, CFStringRef notification);

 private:
  // Add one notification to the list of notifications monitored by our
  // observer.
  void AddNotification(NSString* notification);

  // Convenience function to get the value of an AX attribute from
  // an AXUIElementRef as a string.
  std::string GetAXAttributeValue(
      AXUIElementRef element, NSString* attribute_name);

  // The AXUIElement for the Chrome application.
  base::ScopedCFTypeRef<AXUIElementRef> application_;

  // The AXObserver we use to monitor AX notifications.
  base::ScopedCFTypeRef<AXObserverRef> observer_ref_;
  CFRunLoopSourceRef observer_run_loop_source_;
};

// Callback function registered using AXObserverCreate.
static void EventReceivedThunk(
    AXObserverRef observer_ref,
    AXUIElementRef element,
    CFStringRef notification,
    void *refcon) {
  AccessibilityEventRecorderMac* this_ptr =
      static_cast<AccessibilityEventRecorderMac*>(refcon);
  this_ptr->EventReceived(element, notification);
}

// static
AccessibilityEventRecorder* AccessibilityEventRecorder::Create(
    BrowserAccessibilityManager* manager,
    base::ProcessId pid) {
  return new AccessibilityEventRecorderMac(manager, pid);
}

AccessibilityEventRecorderMac::AccessibilityEventRecorderMac(
    BrowserAccessibilityManager* manager,
    base::ProcessId pid)
    : AccessibilityEventRecorder(manager, pid),
      observer_run_loop_source_(NULL) {
  if (kAXErrorSuccess != AXObserverCreate(pid, EventReceivedThunk,
                                          observer_ref_.InitializeInto())) {
    LOG(FATAL) << "Failed to create AXObserverRef";
  }

  // Get an AXUIElement for the Chrome application.
  application_.reset(AXUIElementCreateApplication(pid));
  if (!application_.get())
    LOG(FATAL) << "Failed to create AXUIElement for application.";

  // Add the notifications we care about to the observer.
  AddNotification(@"AXAutocorrectionOccurred");
  AddNotification(@"AXExpandedChanged");
  AddNotification(@"AXInvalidStatusChanged");
  AddNotification(@"AXLiveRegionChanged");
  AddNotification(@"AXLiveRegionCreated");
  AddNotification(@"AXLoadComplete");
  AddNotification(@"AXMenuItemSelected");
  AddNotification(@"AXRowCollapsed");
  AddNotification(@"AXRowExpanded");
  AddNotification(NSAccessibilityFocusedUIElementChangedNotification);
  AddNotification(NSAccessibilityRowCollapsedNotification);
  AddNotification(NSAccessibilityRowCountChangedNotification);
  AddNotification(NSAccessibilitySelectedChildrenChangedNotification);
  AddNotification(NSAccessibilitySelectedRowsChangedNotification);
  AddNotification(NSAccessibilitySelectedTextChangedNotification);
  AddNotification(NSAccessibilityValueChangedNotification);

  // Add the observer to the current message loop.
  observer_run_loop_source_ = AXObserverGetRunLoopSource(observer_ref_.get());
  CFRunLoopAddSource(CFRunLoopGetCurrent(), observer_run_loop_source_,
                     kCFRunLoopDefaultMode);
}

AccessibilityEventRecorderMac::~AccessibilityEventRecorderMac() {
  CFRunLoopRemoveSource(CFRunLoopGetCurrent(), observer_run_loop_source_,
                        kCFRunLoopDefaultMode);
}

void AccessibilityEventRecorderMac::AddNotification(NSString* notification) {
  AXObserverAddNotification(observer_ref_, application_,
                            base::mac::NSToCFCast(notification), this);
}

std::string AccessibilityEventRecorderMac::GetAXAttributeValue(
    AXUIElementRef element,
    NSString* attribute_name) {
  base::ScopedCFTypeRef<CFTypeRef> value;
  AXError err = AXUIElementCopyAttributeValue(
      element, base::mac::NSToCFCast(attribute_name), value.InitializeInto());
  if (err != kAXErrorSuccess)
    return std::string();

  CFStringRef value_string = base::mac::CFCast<CFStringRef>(value.get());
  if (value_string)
    return base::SysCFStringRefToUTF8(value_string);

  // TODO(dmazzoni): And if it's not a string, can we return something better?
  return std::string();
}

void AccessibilityEventRecorderMac::EventReceived(AXUIElementRef element,
                                                  CFStringRef notification) {
  std::string notification_str = base::SysCFStringRefToUTF8(notification);
  std::string role = GetAXAttributeValue(element, NSAccessibilityRoleAttribute);
  if (role.empty())
    return;
  std::string log =
      base::StringPrintf("%s on %s", notification_str.c_str(), role.c_str());

  std::string title =
      GetAXAttributeValue(element, NSAccessibilityTitleAttribute);
  if (!title.empty())
    log += base::StringPrintf(" AXTitle=\"%s\"", title.c_str());

  std::string description =
      GetAXAttributeValue(element, NSAccessibilityDescriptionAttribute);
  if (!description.empty())
    log += base::StringPrintf(" AXDescription=\"%s\"", description.c_str());

  std::string value =
      GetAXAttributeValue(element, NSAccessibilityValueAttribute);
  if (!value.empty())
    log += base::StringPrintf(" AXValue=\"%s\"", value.c_str());

  OnEvent(log);
}

}  // namespace content
