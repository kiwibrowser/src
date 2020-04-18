// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/crash_report/crash_report_helper.h"

#import <Foundation/Foundation.h>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/debug/crash_logging.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "components/upload_list/crash_upload_list.h"
#include "ios/chrome/browser/chrome_paths.h"
#include "ios/chrome/browser/crash_report/breakpad_helper.h"
#import "ios/chrome/browser/crash_report/crash_report_user_application_state.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/tabs/tab_model_observer.h"
#import "ios/chrome/browser/web/tab_id_tab_helper.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#include "ios/web/public/browser_state.h"
#import "ios/web/public/navigation_item.h"
#include "ios/web/public/web_state/web_state.h"
#include "ios/web/public/web_thread.h"
#import "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// TabModelObserver that allows loaded urls to be sent to the crash server.
@interface CrashReporterURLObserver : NSObject<TabModelObserver> {
 @private
  // Map associating the tab id to the breakpad key used to keep track of the
  // loaded URL.
  NSMutableDictionary* breakpadKeyByTabId_;
  // List of keys to use for recording URLs. This list is sorted such that a new
  // tab must use the first key in this list to record its URLs.
  NSMutableArray* breakpadKeys_;
}
+ (CrashReporterURLObserver*)uniqueInstance;
// Removes the URL for the tab with the given id from the URLs sent to the crash
// server.
- (void)removeTabId:(NSString*)tabId;
// Records the given URL associated to the given id to the list of URLs to send
// to the crash server. If |pending| is true, the URL is one that is
// expected to start loading, but hasn't actually been seen yet.
- (void)recordURL:(NSString*)url
         forTabId:(NSString*)tabId
          pending:(BOOL)pending;
// Callback for the kTabUrlStartedLoadingNotificationForCrashReporting
// notification. Extracts the parameter from the notification and calls
// |recordURL:forTabId:pending:|.
- (void)urlChanged:(NSNotification*)notification;
// Callback for the kTabUrlMayStartLoadingNotificationForCrashReporting
// notification. Extracts the parameter from the notification and calls
// |recordURL:forTabId:pending:|.
- (void)urlChangeExpected:(NSNotification*)notification;
@end

// TabModelObserver that some tabs stats to be sent to the crash server.
@interface CrashReporterTabStateObserver : NSObject<TabModelObserver> {
 @private
  // Map associating the tab id to an object describing the current state of the
  // tab.
  NSMutableDictionary* tabCurrentStateByTabId_;
}
+ (CrashReporterURLObserver*)uniqueInstance;
// Removes the stats for the tab tabId
- (void)removeTabId:(NSString*)tabId;
// Callback for the kTabClosingCurrentDocumentNotificationForCrashReporting
// notification. Removes document related information from
// tabCurrentStateByTabId_ by calling closingDocumentInTab:tabId.
- (void)closingDocument:(NSNotification*)notification;
// Removes document related information from tabCurrentStateByTabId_.
- (void)closingDocumentInTab:(NSString*)tabId;
// Callback for the  kTabIsShowingExportableNotificationForCrashReporting
// notification. Sets the mimeType in tabCurrentStateByTabId_.
- (void)showingExportableDocument:(NSNotification*)notification;

// Sets a tab |tabId| specific information with key |key| and value |value| in
// tabCurrentStateByTabId_.
- (void)setTabInfo:(NSString*)key
         withValue:(NSString*)value
            forTab:(NSString*)tabId;
// Retrieves the |key| information for tab |tabId|.
- (id)getTabInfo:(NSString*)key forTab:(NSString*)tabId;
// Removes the |key| information for tab |tabId|
- (void)removeTabInfo:(NSString*)key forTab:(NSString*)tabId;
@end

namespace {

// Returns the breakpad key to use for a pending URL corresponding to the
// same tab that is using |key|.
NSString* PendingURLKeyForKey(NSString* key) {
  return [key stringByAppendingString:@"-pending"];
}

// Max number of urls to send. This must be kept low for privacy issue as well
// as because breakpad does limit the total number of parameters to 64.
const int kNumberOfURLsToSend = 1;
}

@implementation CrashReporterURLObserver

+ (CrashReporterURLObserver*)uniqueInstance {
  static CrashReporterURLObserver* instance =
      [[CrashReporterURLObserver alloc] init];
  return instance;
}

- (id)init {
  if ((self = [super init])) {
    breakpadKeyByTabId_ =
        [[NSMutableDictionary alloc] initWithCapacity:kNumberOfURLsToSend];
    breakpadKeys_ =
        [[NSMutableArray alloc] initWithCapacity:kNumberOfURLsToSend];
    for (int i = 0; i < kNumberOfURLsToSend; ++i)
      [breakpadKeys_ addObject:[NSString stringWithFormat:@"url%d", i]];
    // Register for url changed notifications.
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(urlChanged:)
               name:kTabUrlStartedLoadingNotificationForCrashReporting
             object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(urlChangeExpected:)
               name:kTabUrlMayStartLoadingNotificationForCrashReporting
             object:nil];
  }
  return self;
}

- (void)urlChanged:(NSNotification*)notification {
  Tab* tab = notification.object;
  DCHECK(tab);
  if (tab.webState->GetBrowserState()->IsOffTheRecord())
    return;
  NSString* url = [notification.userInfo objectForKey:kTabUrlKey];
  DCHECK(url);
  [self recordURL:url forTabId:tab.tabId pending:NO];
}

- (void)urlChangeExpected:(NSNotification*)notification {
  Tab* tab = notification.object;
  DCHECK(tab);
  if (tab.webState->GetBrowserState()->IsOffTheRecord())
    return;
  NSString* url = [notification.userInfo objectForKey:kTabUrlKey];
  DCHECK(url);
  [self recordURL:url forTabId:tab.tabId pending:YES];
}

- (void)removeTabId:(NSString*)tabId {
  NSString* key = [breakpadKeyByTabId_ objectForKey:tabId];
  if (!key)
    return;
  breakpad_helper::RemoveReportParameter(key);
  breakpad_helper::RemoveReportParameter(PendingURLKeyForKey(key));
  [breakpadKeyByTabId_ removeObjectForKey:tabId];
  [breakpadKeys_ removeObject:key];
  [breakpadKeys_ insertObject:key atIndex:0];
}

- (void)recordURL:(NSString*)url
         forTabId:(NSString*)tabId
          pending:(BOOL)pending {
  NSString* breakpadKey = [breakpadKeyByTabId_ objectForKey:tabId];
  BOOL reusingKey = NO;
  if (!breakpadKey) {
    // Get the first breakpad key and push it back at the end of the keys.
    breakpadKey = [breakpadKeys_ objectAtIndex:0];
    [breakpadKeys_ removeObject:breakpadKey];
    [breakpadKeys_ addObject:breakpadKey];
    // Remove the current mapping to the breakpad key.
    for (NSString* tabId in
         [breakpadKeyByTabId_ allKeysForObject:breakpadKey]) {
      reusingKey = YES;
      [breakpadKeyByTabId_ removeObjectForKey:tabId];
    }
    // Associate the breakpad key to the tab id.
    [breakpadKeyByTabId_ setObject:breakpadKey forKey:tabId];
  }
  NSString* pendingKey = PendingURLKeyForKey(breakpadKey);
  if (pending) {
    if (reusingKey)
      breakpad_helper::RemoveReportParameter(breakpadKey);
    breakpad_helper::AddReportParameter(pendingKey, url, true);
  } else {
    breakpad_helper::AddReportParameter(breakpadKey, url, true);
    breakpad_helper::RemoveReportParameter(pendingKey);
  }
}

- (void)tabModel:(TabModel*)model
    didRemoveTab:(Tab*)tab
         atIndex:(NSUInteger)index {
  [self removeTabId:tab.tabId];
}

- (void)tabModel:(TabModel*)model
    didReplaceTab:(Tab*)oldTab
          withTab:(Tab*)newTab
          atIndex:(NSUInteger)index {
  [self removeTabId:oldTab.tabId];
}

- (void)tabModel:(TabModel*)model
    didChangeActiveTab:(Tab*)newTab
           previousTab:(Tab*)previousTab
               atIndex:(NSUInteger)modelIndex {
  web::NavigationItem* pendingItem =
      newTab.webState->GetNavigationManager()->GetPendingItem();
  const GURL& URL = pendingItem ? pendingItem->GetURL()
                                : newTab.webState->GetLastCommittedURL();
  [self recordURL:base::SysUTF8ToNSString(URL.spec())
         forTabId:newTab.tabId
          pending:pendingItem ? YES : NO];
}

// Empty method left in place in case jailbreakers are swizzling this.
- (void)detectJailbrokenDevice {
  // This method has been intentionally left blank.
}

@end

@implementation CrashReporterTabStateObserver

+ (CrashReporterTabStateObserver*)uniqueInstance {
  static CrashReporterTabStateObserver* instance =
      [[CrashReporterTabStateObserver alloc] init];
  return instance;
}

- (id)init {
  if ((self = [super init])) {
    tabCurrentStateByTabId_ = [[NSMutableDictionary alloc] init];
    // Register for url changed notifications.
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(closingDocument:)
               name:kTabClosingCurrentDocumentNotificationForCrashReporting
             object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(showingExportableDocument:)
               name:kTabIsShowingExportableNotificationForCrashReporting
             object:nil];
  }
  return self;
}

- (void)closingDocument:(NSNotification*)notification {
  Tab* tab = notification.object;
  [self closingDocumentInTab:[tab tabId]];
}

- (void)closingDocumentInTab:(NSString*)tabId {
  NSString* mime = (NSString*)[self getTabInfo:@"mime" forTab:tabId];
  if ([mime isEqualToString:@"application/pdf"])
    breakpad_helper::SetCurrentTabIsPDF(false);
  [self removeTabInfo:@"mime" forTab:tabId];
}

- (void)setTabInfo:(NSString*)key
         withValue:(NSString*)value
            forTab:(NSString*)tabId {
  NSMutableDictionary* tabCurrentState =
      [tabCurrentStateByTabId_ objectForKey:tabId];
  if (tabCurrentState == nil) {
    NSMutableDictionary* currentStateOfNewTab =
        [[NSMutableDictionary alloc] init];
    [tabCurrentStateByTabId_ setObject:currentStateOfNewTab forKey:tabId];
    tabCurrentState = [tabCurrentStateByTabId_ objectForKey:tabId];
  }
  [tabCurrentState setObject:value forKey:key];
}

- (id)getTabInfo:(NSString*)key forTab:(NSString*)tabId {
  NSMutableDictionary* tabValues = [tabCurrentStateByTabId_ objectForKey:tabId];
  return [tabValues objectForKey:key];
}

- (void)removeTabInfo:(NSString*)key forTab:(NSString*)tabId {
  [[tabCurrentStateByTabId_ objectForKey:tabId] removeObjectForKey:key];
}

- (void)showingExportableDocument:(NSNotification*)notification {
  Tab* tab = notification.object;
  NSString* oldMime = (NSString*)[self getTabInfo:@"mime" forTab:[tab tabId]];
  if ([oldMime isEqualToString:@"application/pdf"])
    return;

  std::string mime = [tab webState]->GetContentsMimeType();
  NSString* nsMime = base::SysUTF8ToNSString(mime);
  [self setTabInfo:@"mime" withValue:nsMime forTab:[tab tabId]];
  breakpad_helper::SetCurrentTabIsPDF(true);
}

- (void)removeTabId:(NSString*)tabId {
  [self closingDocumentInTab:tabId];
  [tabCurrentStateByTabId_ removeObjectForKey:tabId];
}

- (void)tabModel:(TabModel*)model
    didRemoveTab:(Tab*)tab
         atIndex:(NSUInteger)index {
  [self removeTabId:tab.tabId];
}

- (void)tabModel:(TabModel*)model
    didReplaceTab:(Tab*)oldTab
          withTab:(Tab*)newTab
          atIndex:(NSUInteger)index {
  [self removeTabId:oldTab.tabId];
}

@end

namespace breakpad {

void MonitorURLsForTabModel(TabModel* tab_model) {
  DCHECK(!tab_model.isOffTheRecord);
  [tab_model addObserver:[CrashReporterURLObserver uniqueInstance]];
}

void StopMonitoringURLsForTabModel(TabModel* tab_model) {
  [tab_model removeObserver:[CrashReporterURLObserver uniqueInstance]];
}

void MonitorTabStateForTabModel(TabModel* tab_model) {
  [tab_model addObserver:[CrashReporterTabStateObserver uniqueInstance]];
}

void StopMonitoringTabStateForTabModel(TabModel* tab_model) {
  [tab_model removeObserver:[CrashReporterTabStateObserver uniqueInstance]];
}

void ClearStateForTabModel(TabModel* tab_model) {
  CrashReporterURLObserver* observer =
      [CrashReporterURLObserver uniqueInstance];

  WebStateList* web_state_list = tab_model.webStateList;
  for (int index = 0; index < web_state_list->count(); ++index) {
    web::WebState* web_state = web_state_list->GetWebStateAt(index);
    [observer removeTabId:TabIdTabHelper::FromWebState(web_state)->tab_id()];
  }
}

}  // namespace breakpad
