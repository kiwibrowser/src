// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_CRASH_REPORT_CRASH_REPORT_HELPER_H_
#define IOS_CHROME_BROWSER_CRASH_REPORT_CRASH_REPORT_HELPER_H_

@class NSString;
@class TabModel;

namespace breakpad {

// Monitors the urls loaded in |tab_model| to allow crash reports to contain
// the currently loaded urls.
// |tab_model| must not be an off-the-record tab model.
void MonitorURLsForTabModel(TabModel* tab_model);

// Stop monitoring the urls loaded in the |tabModel|.
void StopMonitoringURLsForTabModel(TabModel* tab_model);

// Adds the state monitor to |tab_model|. TabModels that are not monitored via
// this function are still monitored through notifications, but calling this
// function is mandatory to keep the monitoring of deleted tabs consistent.
void MonitorTabStateForTabModel(TabModel* tab_model);

// Stop the state monitor of |tab_model|.
void StopMonitoringTabStateForTabModel(TabModel* tab_model);

// Clear any state about the urls loaded in the given TabModel; this should be
// called when the tab model is deactivated.
void ClearStateForTabModel(TabModel* tab_model);

}  // namespace breakpad

#endif  // IOS_CHROME_BROWSER_CRASH_REPORT_CRASH_REPORT_HELPER_H_
