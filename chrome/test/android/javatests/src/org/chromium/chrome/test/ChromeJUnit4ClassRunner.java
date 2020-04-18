// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.text.TextUtils;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;

import org.junit.runners.model.InitializationError;

import org.chromium.base.CollectionUtil;
import org.chromium.base.CommandLine;
import org.chromium.base.CommandLineInitUtil;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.BaseTestResult.PreTestHook;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.RestrictionSkipCheck;
import org.chromium.base.test.util.SkipCheck;
import org.chromium.chrome.browser.ChromeVersionInfo;
import org.chromium.chrome.browser.vr_shell.VrShellDelegate;
import org.chromium.chrome.test.util.ChromeRestriction;
import org.chromium.content.browser.test.ChildProcessAllocatorSettingsHook;
import org.chromium.policy.test.annotations.Policies;
import org.chromium.ui.test.util.UiDisableIfSkipCheck;
import org.chromium.ui.test.util.UiRestrictionSkipCheck;

import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;

/**
 * A custom runner for //chrome JUnit4 tests.
 */
public class ChromeJUnit4ClassRunner extends BaseJUnit4ClassRunner {
    /**
     * Create a ChromeJUnit4ClassRunner to run {@code klass} and initialize values
     *
     * @throws InitializationError if the test class malformed
     */
    public ChromeJUnit4ClassRunner(final Class<?> klass) throws InitializationError {
        super(klass, defaultSkipChecks(), defaultPreTestHooks());
    }

    private static List<SkipCheck> defaultSkipChecks() {
        return CollectionUtil.newArrayList(
                new ChromeRestrictionSkipCheck(InstrumentationRegistry.getTargetContext()),
                new UiRestrictionSkipCheck(InstrumentationRegistry.getTargetContext()),
                new UiDisableIfSkipCheck(InstrumentationRegistry.getTargetContext()));
    }

    private static List<PreTestHook> defaultPreTestHooks() {
        return CollectionUtil.newArrayList(CommandLineFlags.getRegistrationHook(),
                new ChildProcessAllocatorSettingsHook(), Policies.getRegistrationHook());
    }

    @Override
    protected void initCommandLineForTest() {
        CommandLineInitUtil.initCommandLine(CommandLineFlags.getTestCmdLineFile());
    }

    private static class ChromeRestrictionSkipCheck extends RestrictionSkipCheck {
        public ChromeRestrictionSkipCheck(Context targetContext) {
            super(targetContext);
        }

        private boolean isDaydreamReady() {
            return VrShellDelegate.isDaydreamReadyDevice();
        }

        private boolean isDaydreamViewPaired() {
            if (VrShellDelegate.getVrDaydreamApi() == null) {
                return false;
            }
            // isDaydreamCurrentViewer() creates a concrete instance of DaydreamApi,
            // which can only be done on the main thread
            FutureTask<Boolean> checker = new FutureTask<>(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return VrShellDelegate.getVrDaydreamApi().isDaydreamCurrentViewer();
                }
            });
            ThreadUtils.runOnUiThreadBlocking(checker);
            try {
                return checker.get().booleanValue();
            } catch (CancellationException | InterruptedException | ExecutionException
                    | IllegalArgumentException e) {
                return false;
            }
        }

        private boolean isDonEnabled() {
            // We can't directly check whether the VR DON flow is enabled since
            // we don't have permission to read the VrCore settings file. Instead,
            // pass a flag.
            return CommandLine.getInstance().hasSwitch("don-enabled");
        }

        @Override
        protected boolean restrictionApplies(String restriction) {
            if (TextUtils.equals(
                        restriction, ChromeRestriction.RESTRICTION_TYPE_GOOGLE_PLAY_SERVICES)
                    && (ConnectionResult.SUCCESS
                               != GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(
                                          getTargetContext()))) {
                return true;
            }
            if (TextUtils.equals(restriction, ChromeRestriction.RESTRICTION_TYPE_OFFICIAL_BUILD)
                    && (!ChromeVersionInfo.isOfficialBuild())) {
                return true;
            }
            if (TextUtils.equals(restriction, ChromeRestriction.RESTRICTION_TYPE_DEVICE_DAYDREAM)
                    || TextUtils.equals(restriction,
                               ChromeRestriction.RESTRICTION_TYPE_DEVICE_NON_DAYDREAM)) {
                boolean isDaydream = isDaydreamReady();
                if (TextUtils.equals(
                            restriction, ChromeRestriction.RESTRICTION_TYPE_DEVICE_DAYDREAM)
                        && !isDaydream) {
                    return true;
                } else if (TextUtils.equals(restriction,
                                   ChromeRestriction.RESTRICTION_TYPE_DEVICE_NON_DAYDREAM)
                        && isDaydream) {
                    return true;
                }
            }
            if (TextUtils.equals(restriction, ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM)
                    || TextUtils.equals(restriction,
                               ChromeRestriction.RESTRICTION_TYPE_VIEWER_NON_DAYDREAM)) {
                boolean daydreamViewPaired = isDaydreamViewPaired();
                if (TextUtils.equals(
                            restriction, ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM)
                        && !daydreamViewPaired) {
                    return true;
                } else if (TextUtils.equals(restriction,
                                   ChromeRestriction.RESTRICTION_TYPE_VIEWER_NON_DAYDREAM)
                        && daydreamViewPaired) {
                    return true;
                }
            }
            if (TextUtils.equals(restriction, ChromeRestriction.RESTRICTION_TYPE_DON_ENABLED)) {
                return !isDonEnabled();
            }
            return false;
        }
    }
}
