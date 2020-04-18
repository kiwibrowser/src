// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_INTENT_HELPER_ARC_EXTERNAL_PROTOCOL_DIALOG_H_
#define CHROME_BROWSER_CHROMEOS_ARC_INTENT_HELPER_ARC_EXTERNAL_PROTOCOL_DIALOG_H_

#include <string>
#include <utility>
#include <vector>

#include "components/arc/common/intent_helper.mojom.h"
#include "components/arc/intent_helper/arc_intent_helper_bridge.h"
#include "ui/base/page_transition_types.h"

class GURL;

namespace content {
class WebContents;
}  // namespace content

namespace arc {

using GurlAndActivityInfo =
    std::pair<GURL, ArcIntentHelperBridge::ActivityName>;

// An enum returned from GetAction function. This is visible for testing.
enum class GetActionResult {
  // ARC cannot handle the |original_url|, and the URL does not have a fallback
  // http(s) URL. Chrome should show the "Google Chrome OS can't open the page"
  // dialog now.
  SHOW_CHROME_OS_DIALOG,
  // ARC cannot handle the |original_url|, but the URL did have a fallback URL
  // which Chrome can handle. Chrome should show the fallback URL now.
  OPEN_URL_IN_CHROME,
  // ARC can handle the |original_url|, and one of the ARC activities is a
  // preferred one. ARC should handle the URL now.
  HANDLE_URL_IN_ARC,
  // Chrome should show the disambig UI because 1) ARC can handle the
  // |original_url| but none of the ARC activities is a preferred one, or
  // 2) there are two or more browsers (e.g. Chrome and a browser app in ARC)
  // that can handle a fallback URL.
  ASK_USER,
};

// Shows ARC version of the dialog. Returns true if ARC is supported, running,
// and in a context where it is allowed to handle external protocol.
bool RunArcExternalProtocolDialog(const GURL& url,
                                  int render_process_host_id,
                                  int routing_id,
                                  ui::PageTransition page_transition,
                                  bool has_user_gesture);

GetActionResult GetActionForTesting(
    const GURL& original_url,
    const std::vector<mojom::IntentHandlerInfoPtr>& handlers,
    size_t selected_app_index,
    GurlAndActivityInfo* out_url_and_activity_name,
    bool* safe_to_bypass_ui);

GURL GetUrlToNavigateOnDeactivateForTesting(
    const std::vector<mojom::IntentHandlerInfoPtr>& handlers);

bool GetAndResetSafeToRedirectToArcWithoutUserConfirmationFlagForTesting(
    content::WebContents* web_contents);

bool IsChromeAnAppCandidateForTesting(
    const std::vector<mojom::IntentHandlerInfoPtr>& handlers);

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_INTENT_HELPER_ARC_EXTERNAL_PROTOCOL_DIALOG_H_
