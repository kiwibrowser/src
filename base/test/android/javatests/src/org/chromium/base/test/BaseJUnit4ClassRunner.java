// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test;

import static org.chromium.base.test.BaseChromiumAndroidJUnitRunner.shouldListTests;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.internal.runner.junit4.AndroidJUnit4ClassRunner;
import android.support.test.internal.util.AndroidRunnerParams;

import org.junit.runner.Description;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.Statement;

import org.chromium.base.CollectionUtil;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.test.BaseTestResult.PreTestHook;
import org.chromium.base.test.util.DisableIfSkipCheck;
import org.chromium.base.test.util.ManualSkipCheck;
import org.chromium.base.test.util.MinAndroidSdkLevelSkipCheck;
import org.chromium.base.test.util.RestrictionSkipCheck;
import org.chromium.base.test.util.SkipCheck;

import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

/**
 *  A custom runner for JUnit4 tests that checks requirements to conditionally ignore tests.
 *
 *  This ClassRunner imports from AndroidJUnit4ClassRunner which is a hidden but accessible
 *  class. The reason is that default JUnit4 runner for Android is a final class,
 *  AndroidJUnit4. We need to extends an inheritable class to change {@link #runChild}
 *  and {@link #isIgnored} to add SkipChecks and PreTesthook.
 */
public class BaseJUnit4ClassRunner extends AndroidJUnit4ClassRunner {
    private static final String TAG = "BaseJUnit4ClassRunnr";
    private final List<SkipCheck> mSkipChecks;
    private final List<PreTestHook> mPreTestHooks;

    private static final String EXTRA_TRACE_FILE =
            "org.chromium.base.test.BaseJUnit4ClassRunner.TraceFile";

    /**
     * Create a BaseJUnit4ClassRunner to run {@code klass} and initialize values
     *
     * @throws InitializationError if the test class malformed
     */
    public BaseJUnit4ClassRunner(final Class<?> klass) throws InitializationError {
        this(klass, null, null);
    }

    /**
     * Create a BaseJUnit4ClassRunner to run {@code klass} and initialize values.
     *
     * To add more SkipCheck or PreTestHook in subclass, create Lists of checks and hooks,
     * pass them into the super constructors. If you want make a subclass extendable by other
     * class runners, you also have to create a constructor similar to the following one that
     * merges default checks or hooks with this checks and hooks passed in by constructor.
     *
     * <pre>
     * <code>
     * e.g.
     * public ChildRunner extends BaseJUnit4ClassRunner {
     *     public ChildRunner(final Class<?> klass) {
     *             throws InitializationError {
     *         this(klass, null, null);
     *     }
     *
     *     public ChildRunner(
     *             final Class<?> klass, List<SkipCheck> checks, List<PreTestHook> hook) {
     *             throws InitializationError {
     *         super(klass, mergeList(
     *             checks, defaultSkipChecks()), mergeList(hooks, DEFAULT_HOOKS));
     *     }
     *
     *     public List<SkipCheck> defaultSkipChecks() {...}
     *
     *     public List<PreTestHook> defaultPreTestHooks() {...}
     * </code>
     * </pre>
     *
     * @throws InitializationError if the test class malformed
     */
    public BaseJUnit4ClassRunner(
            final Class<?> klass, List<SkipCheck> checks, List<PreTestHook> hooks)
            throws InitializationError {
        super(klass,
                new AndroidRunnerParams(InstrumentationRegistry.getInstrumentation(),
                        InstrumentationRegistry.getArguments(), false, 0L, false));

        String traceOutput = InstrumentationRegistry.getArguments().getString(EXTRA_TRACE_FILE);

        if (traceOutput != null) {
            File traceOutputFile = new File(traceOutput);
            File traceOutputDir = traceOutputFile.getParentFile();

            if (traceOutputDir != null) {
                if (traceOutputDir.exists() || traceOutputDir.mkdirs()) {
                    TestTraceEvent.enable(traceOutputFile);
                }
            }
        }

        mSkipChecks = mergeList(checks, defaultSkipChecks());
        mPreTestHooks = mergeList(hooks, defaultPreTestHooks());
    }

    /**
     * Merge two List into a new ArrayList.
     *
     * Used to merge the default SkipChecks/PreTestHooks with the subclasses's
     * SkipChecks/PreTestHooks.
     */
    protected static final <T> List<T> mergeList(List<T> listA, List<T> listB) {
        List<T> l = new ArrayList<>();
        if (listA != null) {
            l.addAll(listA);
        }
        if (listB != null) {
            l.addAll(listB);
        }
        return l;
    }

    @Override
    protected void collectInitializationErrors(List<Throwable> errors) {
        super.collectInitializationErrors(errors);
        // Log any initialization errors to help debugging, as the host-side test runner can get
        // confused by the thrown exception.
        if (!errors.isEmpty()) {
            Log.e(TAG, "Initialization errors in %s: %s", getTestClass().getName(), errors);
        }
    }

    /**
     * Change this static function to add or take out default {@code SkipCheck}s.
     */
    private static List<SkipCheck> defaultSkipChecks() {
        return CollectionUtil.newArrayList(
                new RestrictionSkipCheck(InstrumentationRegistry.getTargetContext()),
                new MinAndroidSdkLevelSkipCheck(), new DisableIfSkipCheck(), new ManualSkipCheck());
    }

    /**
     * Change this static function to add or take out default {@code PreTestHook}s.
     */
    private static List<PreTestHook> defaultPreTestHooks() {
        return null;
    }

    /**
     * Evaluate whether a FrameworkMethod is ignored based on {@code SkipCheck}s.
     */
    @Override
    protected boolean isIgnored(FrameworkMethod method) {
        return super.isIgnored(method) || shouldSkip(method);
    }

    /**
     * Run test with or without execution based on bundle arguments.
     */
    @Override
    public void run(RunNotifier notifier) {
        ContextUtils.initApplicationContext(
                InstrumentationRegistry.getTargetContext().getApplicationContext());
        if (shouldListTests(InstrumentationRegistry.getArguments())) {
            for (Description child : getDescription().getChildren()) {
                notifier.fireTestStarted(child);
                notifier.fireTestFinished(child);
            }
        } else {
            if (!CommandLine.isInitialized()) {
                initCommandLineForTest();
            }
            super.run(notifier);
        }
    }

    /**
     * Override this method to change how test class runner initiate commandline flags
     */
    protected void initCommandLineForTest() {
        CommandLine.init(null);
    }

    @Override
    protected void runChild(FrameworkMethod method, RunNotifier notifier) {
        String testName = method.getName();
        TestTraceEvent.begin(testName);

        runPreTestHooks(method);

        super.runChild(method, notifier);

        TestTraceEvent.end(testName);

        // A new instance of BaseJUnit4ClassRunner is created on the device
        // for each new method, so runChild will only be called once. Thus, we
        // can disable tracing, and dump the output, once we get here.
        TestTraceEvent.disable();
    }

    /**
     * Loop through all the {@code PreTestHook}s to run them
     */
    private void runPreTestHooks(FrameworkMethod frameworkMethod) {
        Method testMethod = frameworkMethod.getMethod();
        Context targetContext = InstrumentationRegistry.getTargetContext();
        for (PreTestHook hook : mPreTestHooks) {
            hook.run(targetContext, testMethod);
        }
    }

    /**
     * Loop through all the {@code SkipCheck}s to confirm whether a test should be ignored
     */
    private boolean shouldSkip(FrameworkMethod method) {
        for (SkipCheck s : mSkipChecks) {
            if (s.shouldSkip(method)) {
                return true;
            }
        }
        return false;
    }

    /*
     * Overriding this method to take screenshot of failure before tear down functions are run.
     */
    @Override
    protected Statement withAfters(FrameworkMethod method, Object test, Statement base) {
        return super.withAfters(method, test, new ScreenshotOnFailureStatement(base));
    }
}
