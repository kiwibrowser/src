// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

/**
 * A fake {@link EnabledStateMonitor} for use in testing. To finish initializing, call
 * {@link #setObserver(Observer)}.
 */
public class FakeEnabledStateMonitor extends EnabledStateMonitor {
    public FakeEnabledStateMonitor() {
        super(null);
    }

    @Override
    protected void init() {
        // Intentionally do nothing.
    }

    @Override
    public void destroy() {
        // Intentionally do nothing.
    }

    /**
     * Sets an observer for testing.
     * @param observer The {@link Observer} to be notified of changes to enabled state.
     */
    public void setObserver(EnabledStateMonitor.Observer observer) {
        mObserver = observer;
        observer.onEnabledStateChanged(true);
    }

    /** Simulates settings state changed. */
    public void onSettingsStateChanged(boolean enabled) {
        sSettingsEnabledForTesting = enabled;
        mObserver.onSettingsStateChanged(enabled);
    }
}
