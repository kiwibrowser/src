// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.preference.PreferenceFragment;
import android.support.annotation.Nullable;
import android.text.SpannableString;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.contextual_suggestions.ContextualSuggestionsBridge;
import org.chromium.chrome.browser.contextual_suggestions.EnabledStateMonitor;
import org.chromium.chrome.browser.signin.AccountSigninActivity;
import org.chromium.chrome.browser.signin.SigninAccessPoint;
import org.chromium.chrome.browser.sync.ui.SyncCustomizationFragment;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;

/**
 * Fragment to manage the Contextual Suggestions preference and to explain to the user what it does.
 */
public class ContextualSuggestionsPreference
        extends PreferenceFragment implements EnabledStateMonitor.Observer {
    static final String PREF_CONTEXTUAL_SUGGESTIONS_SWITCH = "contextual_suggestions_switch";
    private static final String PREF_CONTEXTUAL_SUGGESTIONS_MESSAGE =
            "contextual_suggestions_message";

    private static EnabledStateMonitor sEnabledStateMonitorForTesting;

    private ChromeSwitchPreference mSwitch;
    private EnabledStateMonitor mEnabledStateMonitor;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.contextual_suggestions_preferences);
        getActivity().setTitle(R.string.prefs_contextual_suggestions);

        mSwitch = (ChromeSwitchPreference) findPreference(PREF_CONTEXTUAL_SUGGESTIONS_SWITCH);
        mEnabledStateMonitor = sEnabledStateMonitorForTesting != null
                ? sEnabledStateMonitorForTesting
                : new EnabledStateMonitor(this);
        initialize();
    }

    @Override
    public void onResume() {
        super.onResume();
        updateSwitch();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mEnabledStateMonitor.destroy();
    }

    @Override
    public void onEnabledStateChanged(boolean enabled) {}

    @Override
    public void onSettingsStateChanged(boolean enabled) {
        if (mEnabledStateMonitor != null) updateSwitch();
    }

    /** Helper method to initialize the switch preference and the message preference. */
    private void initialize() {
        final TextMessagePreference message =
                (TextMessagePreference) findPreference(PREF_CONTEXTUAL_SUGGESTIONS_MESSAGE);
        final NoUnderlineClickableSpan span = new NoUnderlineClickableSpan((widget) -> {
            Context context = getActivity();
            if (ChromeSigninController.get().isSignedIn()) {
                Intent intent = PreferencesLauncher.createIntentForSettingsPage(
                        context, SyncCustomizationFragment.class.getName());
                IntentUtils.safeStartActivity(context, intent);
            } else {
                startActivity(AccountSigninActivity.createIntentForDefaultSigninFlow(
                        context, SigninAccessPoint.SETTINGS, false));
            }
        });
        final SpannableString spannable = SpanApplier.applySpans(
                getResources().getString(R.string.contextual_suggestions_message),
                new SpanApplier.SpanInfo("<link>", "</link>", span));
        message.setTitle(spannable);

        updateSwitch();
        mSwitch.setOnPreferenceChangeListener((preference, newValue) -> {
            boolean enabled = (boolean) newValue;
            PrefServiceBridge.getInstance().setBoolean(
                    Pref.CONTEXTUAL_SUGGESTIONS_ENABLED, enabled);

            EnabledStateMonitor.recordPreferenceEnabled(enabled);
            if (enabled) {
                RecordUserAction.record("ContextualSuggestions.Preference.Enabled");
            } else {
                RecordUserAction.record("ContextualSuggestions.Preference.Disabled");
            }
            return true;
        });
        mSwitch.setManagedPreferenceDelegate(
                preference -> ContextualSuggestionsBridge.isEnterprisePolicyManaged());
    }

    /** Helper method to update the enabled state of the switch. */
    private void updateSwitch() {
        mSwitch.setEnabled(EnabledStateMonitor.getSettingsEnabled());
        mSwitch.setChecked(EnabledStateMonitor.getEnabledState());
    }

    @VisibleForTesting
    static void setEnabledStateMonitorForTesting(EnabledStateMonitor monitor) {
        sEnabledStateMonitorForTesting = monitor;
    }
}
