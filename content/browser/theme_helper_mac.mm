// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/theme_helper_mac.h"

#import <Cocoa/Cocoa.h>

#include "base/command_line.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/common/renderer.mojom.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_switches.h"

using content::RenderProcessHost;
using content::RenderProcessHostImpl;
using content::ThemeHelperMac;

namespace {

blink::WebScrollbarButtonsPlacement GetButtonPlacement() {
  NSString* scrollbar_variant = [[NSUserDefaults standardUserDefaults]
      objectForKey:@"AppleScrollBarVariant"];
  if ([scrollbar_variant isEqualToString:@"Single"])
    return blink::kWebScrollbarButtonsPlacementSingle;
  else if ([scrollbar_variant isEqualToString:@"DoubleMin"])
    return blink::kWebScrollbarButtonsPlacementDoubleStart;
  else if ([scrollbar_variant isEqualToString:@"DoubleBoth"])
    return blink::kWebScrollbarButtonsPlacementDoubleBoth;
  else
    return blink::kWebScrollbarButtonsPlacementDoubleEnd;
}

void FillScrollbarThemeParams(
    content::mojom::UpdateScrollbarThemeParams* params) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  [defaults synchronize];

  params->initial_button_delay =
      [defaults floatForKey:@"NSScrollerButtonDelay"];
  params->autoscroll_button_delay =
      [defaults floatForKey:@"NSScrollerButtonPeriod"];
  params->jump_on_track_click =
      [defaults boolForKey:@"AppleScrollerPagingBehavior"];
  params->preferred_scroller_style =
      ThemeHelperMac::GetPreferredScrollerStyle();
  params->button_placement = GetButtonPlacement();

  id rubber_band_value = [defaults objectForKey:@"NSScrollViewRubberbanding"];
  params->scroll_view_rubber_banding =
      rubber_band_value ? [rubber_band_value boolValue] : YES;
}

void SendSystemColorsChangedMessage(content::mojom::Renderer* renderer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  [defaults synchronize];

  renderer->OnSystemColorsChanged(
      [[defaults stringForKey:@"AppleAquaColorVariant"] intValue],
      base::SysNSStringToUTF8(
          [defaults stringForKey:@"AppleHighlightedTextColor"]),
      base::SysNSStringToUTF8(
          [defaults stringForKey:@"AppleHighlightColor"]));
}

} // namespace

@interface ScrollbarPrefsObserver : NSObject

+ (void)registerAsObserver;
+ (void)appearancePrefsChanged:(NSNotification*)notification;
+ (void)behaviorPrefsChanged:(NSNotification*)notification;
+ (void)notifyPrefsChangedWithRedraw:(BOOL)redraw;

@end

@implementation ScrollbarPrefsObserver

+ (void)registerAsObserver {
  NSDistributedNotificationCenter* distributedCenter =
      [NSDistributedNotificationCenter defaultCenter];
  [distributedCenter addObserver:self
                        selector:@selector(appearancePrefsChanged:)
                            name:@"AppleAquaScrollBarVariantChanged"
                          object:nil
               suspensionBehavior:
                   NSNotificationSuspensionBehaviorDeliverImmediately];

  [distributedCenter addObserver:self
                        selector:@selector(behaviorPrefsChanged:)
                            name:@"AppleNoRedisplayAppearancePreferenceChanged"
                          object:nil
              suspensionBehavior:NSNotificationSuspensionBehaviorCoalesce];

  [distributedCenter addObserver:self
                        selector:@selector(behaviorPrefsChanged:)
                            name:@"NSScrollAnimationEnabled"
                          object:nil
              suspensionBehavior:NSNotificationSuspensionBehaviorCoalesce];

  [distributedCenter addObserver:self
                        selector:@selector(appearancePrefsChanged:)
                            name:@"AppleScrollBarVariant"
                          object:nil
              suspensionBehavior:
                  NSNotificationSuspensionBehaviorDeliverImmediately];

  [distributedCenter
             addObserver:self
                selector:@selector(behaviorPrefsChanged:)
                    name:@"NSScrollViewRubberbanding"
                  object:nil
      suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];

  // In single-process mode, renderers will catch these notifications
  // themselves and listening for them here may trigger the DCHECK in Observe().
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSingleProcess)) {
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];

    [center addObserver:self
               selector:@selector(behaviorPrefsChanged:)
                   name:NSPreferredScrollerStyleDidChangeNotification
                 object:nil];

    [center addObserver:self
               selector:@selector(systemColorsChanged:)
                   name:NSSystemColorsDidChangeNotification
                 object:nil];
  }
}

+ (void)appearancePrefsChanged:(NSNotification*)notification {
  [self notifyPrefsChangedWithRedraw:YES];
}

+ (void)behaviorPrefsChanged:(NSNotification*)notification {
  [self notifyPrefsChangedWithRedraw:NO];
}

+ (void)systemColorsChanged:(NSNotification*)notification {
  for (RenderProcessHost::iterator it(RenderProcessHost::AllHostsIterator());
       !it.IsAtEnd();
       it.Advance()) {
    SendSystemColorsChangedMessage(
        it.GetCurrentValue()->GetRendererInterface());
  }
}

+ (void)notifyPrefsChangedWithRedraw:(BOOL)redraw {
  for (RenderProcessHost::iterator it(RenderProcessHost::AllHostsIterator());
       !it.IsAtEnd();
       it.Advance()) {
    content::mojom::UpdateScrollbarThemeParamsPtr params =
        content::mojom::UpdateScrollbarThemeParams::New();
    FillScrollbarThemeParams(params.get());
    params->redraw = redraw;
    RenderProcessHostImpl* rphi =
        static_cast<RenderProcessHostImpl*>(it.GetCurrentValue());
    rphi->RecomputeAndUpdateWebKitPreferences();
    rphi->GetRendererInterface()->UpdateScrollbarTheme(std::move(params));
  }
}

@end

namespace content {

// static
ThemeHelperMac* ThemeHelperMac::GetInstance() {
  return base::Singleton<ThemeHelperMac,
                         base::LeakySingletonTraits<ThemeHelperMac>>::get();
}

// static
blink::ScrollerStyle ThemeHelperMac::GetPreferredScrollerStyle() {
  return static_cast<blink::ScrollerStyle>([NSScroller preferredScrollerStyle]);
}

ThemeHelperMac::ThemeHelperMac() {
  [ScrollbarPrefsObserver registerAsObserver];
  registrar_.Add(this,
                 NOTIFICATION_RENDERER_PROCESS_CREATED,
                 NotificationService::AllSources());
}

ThemeHelperMac::~ThemeHelperMac() {
}

void ThemeHelperMac::Observe(int type,
                             const NotificationSource& source,
                             const NotificationDetails& details) {
  DCHECK_EQ(NOTIFICATION_RENDERER_PROCESS_CREATED, type);

  // When a new RenderProcess is created, send it the initial preference
  // parameters.
  content::mojom::UpdateScrollbarThemeParamsPtr params =
      content::mojom::UpdateScrollbarThemeParams::New();
  FillScrollbarThemeParams(params.get());
  params->redraw = false;

  RenderProcessHostImpl* rphi =
      Source<content::RenderProcessHostImpl>(source).ptr();
  rphi->RecomputeAndUpdateWebKitPreferences();

  content::mojom::Renderer* renderer = rphi->GetRendererInterface();
  renderer->UpdateScrollbarTheme(std::move(params));
  SendSystemColorsChangedMessage(renderer);
}

}  // namespace content
