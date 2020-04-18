// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.test;

import android.support.test.InstrumentationRegistry;

import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.chrome.test.util.ApplicationData;

/**
 * JUnit test rule that just clears app data before each test. Having this as a rule allows us
 * to ensure the order it's executed in with something like RuleChain, allowing us to execute it
 * prior to launching the browser process.
 */
public class ClearAppDataTestRule implements TestRule {
    @Override
    public Statement apply(final Statement base, Description description) {
        return new Statement() {
            @Override
            public void evaluate() throws Throwable {
                ApplicationData.clearAppData(InstrumentationRegistry.getTargetContext());
                base.evaluate();
            }
        };
    }
}