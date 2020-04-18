// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefChangeRegistrar;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.search_engines.TemplateUrlService.TemplateUrlServiceObserver;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.chrome.browser.signin.SigninManager.SignInStateObserver;
import org.chromium.chrome.browser.sync.ProfileSyncService;
import org.chromium.chrome.browser.sync.ProfileSyncService.SyncStateChangedListener;
import org.chromium.chrome.browser.util.AccessibilityUtil;
import org.chromium.components.sync.ModelType;
import org.chromium.components.sync.UploadState;

/**
 * A monitor that is responsible for detecting changes to conditions required for contextual
 * suggestions to be enabled. Alerts its {@link Observer} when state changes.
 */
public class EnabledStateMonitor implements SyncStateChangedListener, SignInStateObserver,
                                            TemplateUrlServiceObserver,
                                            PrefChangeRegistrar.PrefObserver {
    /** An observer to be notified of enabled state changes. **/
    public interface Observer {
        void onEnabledStateChanged(boolean enabled);
        void onSettingsStateChanged(boolean enabled);
    }

    protected static boolean sSettingsEnabledForTesting;

    @VisibleForTesting
    protected Observer mObserver;
    private PrefChangeRegistrar mPrefChangeRegistrar;

    /** Whether contextual suggestions are enabled. */
    private boolean mEnabled;

    /** Whether the user settings for contextual suggestions are enabled. */
    private boolean mSettingsEnabled;

    /**
     * Construct a new {@link EnabledStateMonitor}.
     * @param observer The {@link Observer} to be notified of changes to enabled state.
     */
    public EnabledStateMonitor(Observer observer) {
        mObserver = observer;
        init();
    }

    /**
     * Initializes the EnabledStateMonitor. Intended to encapsulate creating connections to native
     * code, so that this can be easily stubbed out during tests. This method should only be
     * overridden for testing.
     */
    @VisibleForTesting
    protected void init() {
        // Assert that we don't need to check for the search engine promo to avoid needing to check
        // every time the default search engine is updated.
        assert !LocaleManager.getInstance().needToCheckForSearchEnginePromo();

        mPrefChangeRegistrar = new PrefChangeRegistrar();
        mPrefChangeRegistrar.addObserver(Pref.CONTEXTUAL_SUGGESTIONS_ENABLED, this);
        ProfileSyncService.get().addSyncStateChangedListener(this);
        SigninManager.get().addSignInStateObserver(this);
        TemplateUrlService.getInstance().addObserver(this);
        updateEnabledState();
        recordPreferenceEnabled(
                PrefServiceBridge.getInstance().getBoolean(Pref.CONTEXTUAL_SUGGESTIONS_ENABLED));
    }

    /** Destroys the EnabledStateMonitor. */
    public void destroy() {
        mPrefChangeRegistrar.destroy();
        ProfileSyncService.get().removeSyncStateChangedListener(this);
        SigninManager.get().removeSignInStateObserver(this);
        TemplateUrlService.getInstance().removeObserver(this);
    }

    /** @return Whether the user settings for contextual suggestions should be shown. */
    public static boolean shouldShowSettings() {
        return isDSEConditionMet() && !AccessibilityUtil.isAccessibilityEnabled()
                && !ContextualSuggestionsBridge.isEnterprisePolicyManaged();
    }

    /** @return Whether the settings state is currently enabled. */
    public static boolean getSettingsEnabled() {
        if (sSettingsEnabledForTesting) return true;

        ProfileSyncService service = ProfileSyncService.get();

        boolean isUploadToGoogleActive =
                service.getUploadToGoogleState(ModelType.HISTORY_DELETE_DIRECTIVES)
                == UploadState.ACTIVE;
        boolean isAccessibilityEnabled = AccessibilityUtil.isAccessibilityEnabled();
        return isUploadToGoogleActive && isDSEConditionMet() && !isAccessibilityEnabled
                && !ContextualSuggestionsBridge.isEnterprisePolicyManaged();
    }

    /** @return Whether the state is currently enabled. */
    public static boolean getEnabledState() {
        return getSettingsEnabled()
                && PrefServiceBridge.getInstance().getBoolean(Pref.CONTEXTUAL_SUGGESTIONS_ENABLED);
    }

    public static void recordEnabled(boolean enabled) {
        RecordHistogram.recordBooleanHistogram("ContextualSuggestions.EnabledState", enabled);
    }

    public static void recordPreferenceEnabled(boolean enabled) {
        RecordHistogram.recordBooleanHistogram("ContextualSuggestions.Preference.State", enabled);
    }

    /** Called when accessibility mode changes. */
    void onAccessibilityModeChanged() {
        updateEnabledState();
    }

    @Override
    public void syncStateChanged() {
        updateEnabledState();
    }

    @Override
    public void onSignedIn() {
        updateEnabledState();
    }

    @Override
    public void onSignedOut() {
        updateEnabledState();
    }

    @Override
    public void onTemplateURLServiceChanged() {
        updateEnabledState();
    }

    @Override
    public void onPreferenceChange() {
        updateEnabledState();
    }

    /**
     * Updates whether contextual suggestions are enabled. Notifies the observer if the
     * enabled state has changed.
     */
    private void updateEnabledState() {
        boolean previousSettingsState = mSettingsEnabled;
        boolean previousState = mEnabled;

        mSettingsEnabled = getSettingsEnabled();
        mEnabled = getEnabledState();

        if (mSettingsEnabled != previousSettingsState) {
            mObserver.onSettingsStateChanged(mSettingsEnabled);
        }

        if (mEnabled != previousState) {
            mObserver.onEnabledStateChanged(mEnabled);
            recordEnabled(mEnabled);
        }
    }

    private static boolean isDSEConditionMet() {
        boolean hasCompletedSearchEnginePromo =
                LocaleManager.getInstance().hasCompletedSearchEnginePromo();
        boolean isGoogleDSE = TemplateUrlService.getInstance().isDefaultSearchEngineGoogle();
        return !hasCompletedSearchEnginePromo || isGoogleDSE;
    }
}
