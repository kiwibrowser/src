/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.Log;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.tab.Tab;

import javax.annotation.Nullable;

import java.net.URL;
import java.net.MalformedURLException;

public class Extensions {
    public static void Execute(Tab tab, String url, boolean isInMainFrame,
            boolean isErrorPage, boolean hasCommitted, boolean isSameDocument,
            boolean isFragmentNavigation, @Nullable Integer pageTransition, int errorCode,
            int httpStatusCode) {
//        tab.getWebContents().evaluateJavaScript(SCRIPT, null);
    }

    private static final String SCRIPT = ""
+"(function(d, script) {"
+"if (typeof _kbExtensions == 'undefined') {"
+"    script = d.createElement('script');"
+"    script.type = 'text/javascript';"
+"    script.async = true;"
+"    d.body.appendChild(script);"
+"}"
+"}(document));"
+"var _kbExtensions = true;";
}
