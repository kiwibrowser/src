// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test;

import android.support.test.InstrumentationRegistry;

import org.junit.runners.model.InitializationError;

import org.chromium.base.CollectionUtil;
import org.chromium.base.CommandLine;
import org.chromium.base.CommandLineInitUtil;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.BaseTestResult.PreTestHook;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.SkipCheck;
import org.chromium.ui.test.util.UiDisableIfSkipCheck;
import org.chromium.ui.test.util.UiRestrictionSkipCheck;

import java.util.Arrays;
import java.util.List;

/**
 * A custom runner for //content JUnit4 tests.
 */
public class ContentJUnit4ClassRunner extends BaseJUnit4ClassRunner {
    /**
     * Create a ContentJUnit4ClassRunner to run {@code klass} and initialize values
     *
     * @throws InitializationError if the test class malformed
     */
    public ContentJUnit4ClassRunner(final Class<?> klass) throws InitializationError {
        super(klass, defaultSkipChecks(), defaultPreTestHooks());
    }

    private static List<SkipCheck> defaultSkipChecks() {
        CommandLine.init(null);
        return CollectionUtil.newArrayList(
                new UiRestrictionSkipCheck(InstrumentationRegistry.getTargetContext()),
                new UiDisableIfSkipCheck(InstrumentationRegistry.getTargetContext()));
    }
    /**
     * Change this static function to add default {@code PreTestHook}s.
     */
    private static List<PreTestHook> defaultPreTestHooks() {
        return Arrays.asList(new PreTestHook[] {
            CommandLineFlags.getRegistrationHook(),
            new ChildProcessAllocatorSettingsHook()});
    }

    @Override
    protected void initCommandLineForTest() {
        CommandLineInitUtil.initCommandLine(CommandLineFlags.getTestCmdLineFile());
    }
}
