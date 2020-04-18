// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.webapps;

import android.support.annotation.IntDef;

import org.chromium.chrome.browser.util.UrlUtilities;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Defines which URLs are inside a web app scope as well as what to do when user navigates to them.
 */
public enum WebappScopePolicy {
    LEGACY {
        @Override
        public boolean isUrlInScope(WebappInfo info, String url) {
            return UrlUtilities.sameDomainOrHost(info.uri().toString(), url, true);
        }
    },
    STRICT {
        @Override
        public boolean isUrlInScope(WebappInfo info, String url) {
            return UrlUtilities.isUrlWithinScope(url, info.scopeUri().toString());
        }
    };

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({NavigationDirective.NORMAL_BEHAVIOR,
            NavigationDirective.IGNORE_EXTERNAL_INTENT_REQUESTS})
    public @interface NavigationDirective {
        // No special handling.
        int NORMAL_BEHAVIOR = 0;
        // The navigation should stay in the webapp. External intent handlers should be ignored.
        int IGNORE_EXTERNAL_INTENT_REQUESTS = 1;
    }

    /**
     * @return {@code true} if given {@code url} is in scope of a web app as defined by its
     *         {@code WebappInfo}, {@code false} otherwise.
     */
    abstract boolean isUrlInScope(WebappInfo info, String url);

    /** Applies the scope policy for navigation to {@link url}. */
    public @NavigationDirective int applyPolicyForNavigationToUrl(WebappInfo info, String url) {
        if (isUrlInScope(info, url)) return NavigationDirective.IGNORE_EXTERNAL_INTENT_REQUESTS;
        return NavigationDirective.NORMAL_BEHAVIOR;
    }
}
