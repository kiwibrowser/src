// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.contextual_suggestions.EnabledStateMonitor.Observer;
import org.chromium.chrome.browser.contextual_suggestions.FetchHelper.Delegate;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

/**
 * Provides an injection mechanism for dependencies of the contextual_suggestions package.
 *
 * This class is intended to handle creating the instances of the various classes that interact with
 * native code, so that they can be easily swapped out during tests.
 */
class ContextualSuggestionsDependencyFactory {
    private static ContextualSuggestionsDependencyFactory sInstance;

    static ContextualSuggestionsDependencyFactory getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) sInstance = new ContextualSuggestionsDependencyFactory();
        return sInstance;
    }

    @VisibleForTesting
    static void setInstanceForTesting(ContextualSuggestionsDependencyFactory testInstance) {
        if (sInstance != null && testInstance != null) {
            throw new IllegalStateException("A real instance already exists.");
        }
        sInstance = testInstance;
    }

    /**
     * @param profile Profile of the user.
     * @return A {@link ContextualSuggestionsSource} for getting contextual suggestions for
     *         the current user.
     */
    ContextualSuggestionsSource createContextualSuggestionsSource(Profile profile) {
        return new ContextualSuggestionsSource(profile);
    }

    /**
     * @param observer The {@link Observer} to be notified of changes to enabled state.
     * @return An {@link EnabledStateMonitor}.
     */
    EnabledStateMonitor createEnabledStateMonitor(EnabledStateMonitor.Observer observer) {
        return new EnabledStateMonitor(observer);
    }

    /**
     * @param delegate The {@link Delegate} used to fetch suggestions.
     * @param tabModelSelector The {@link TabModelSelector} for the containing Activity.
     * @return A {@link FetchHelper}.
     */
    FetchHelper createFetchHelper(Delegate delegate, TabModelSelector tabModelSelector) {
        return new FetchHelper(delegate, tabModelSelector);
    }
}
