// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

/**
 * Rule that allows mocking native dependencies of the contextual_suggestions package.
 *
 * The Factory members to override should be set before the main test rule is called to initialize
 * the test activity.
 *
 * @see ContextualSuggestionsDependencyFactory
 */
public class ContextualSuggestionsDependenciesRule extends TestWatcher {
    private TestFactory mFactory;

    public TestFactory getFactory() {
        return mFactory;
    }

    public ContextualSuggestionsDependenciesRule(TestFactory factory) {
        mFactory = factory;
    }

    public ContextualSuggestionsDependenciesRule() {
        this(new TestFactory());
    }

    @Override
    protected void starting(Description description) {
        ContextualSuggestionsDependencyFactory.setInstanceForTesting(mFactory);
    }

    @Override
    protected void finished(Description description) {
        ContextualSuggestionsDependencyFactory.setInstanceForTesting(null);
    }

    /**
     * ContextualSuggestionsDependencyFactory that exposes and allows modifying the instances to be
     * injected.
     */
    static class TestFactory extends ContextualSuggestionsDependencyFactory {
        public ContextualSuggestionsSource suggestionsSource;
        public EnabledStateMonitor enabledStateMonitor;
        public FetchHelper fetchHelper;

        public CallbackHelper createSuggestionsSourceCallback = new CallbackHelper();

        @Override
        ContextualSuggestionsSource createContextualSuggestionsSource(Profile profile) {
            createSuggestionsSourceCallback.notifyCalled();

            if (suggestionsSource != null) return suggestionsSource;
            return super.createContextualSuggestionsSource(profile);
        }

        @Override
        EnabledStateMonitor createEnabledStateMonitor(EnabledStateMonitor.Observer observer) {
            if (enabledStateMonitor != null) return enabledStateMonitor;
            return super.createEnabledStateMonitor(observer);
        }

        @Override
        FetchHelper createFetchHelper(
                FetchHelper.Delegate delegate, TabModelSelector tabModelSelector) {
            if (fetchHelper != null) return fetchHelper;
            return super.createFetchHelper(delegate, tabModelSelector);
        }
    }
}
