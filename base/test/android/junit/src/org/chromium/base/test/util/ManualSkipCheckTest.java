// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.isIn;

import android.app.Instrumentation;
import android.content.Context;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ErrorCollector;
import org.junit.rules.ExternalResource;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunListener;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.model.FrameworkMethod;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.BaseRobolectricTestRunner;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for skipping tests annotated with {@code @}{@link Manual}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ManualSkipCheckTest {
    /**
     * Example test class.
     */
    public static class ManualTest {
        @Test
        @Manual
        public void manualTest() {}

        @Test
        public void nonManualTest() {}
    }

    @Test
    public void testManual() throws NoSuchMethodException {
        FrameworkMethod method = new FrameworkMethod(ManualTest.class.getMethod("manualTest"));
        Assert.assertTrue(new ManualSkipCheck().shouldSkip(method));
    }

    @Test
    public void testNonManual() throws NoSuchMethodException {
        FrameworkMethod method = new FrameworkMethod(ManualTest.class.getMethod("nonManualTest"));
        Assert.assertFalse(new ManualSkipCheck().shouldSkip(method));
    }

    private static class MockRunListener extends RunListener {
        private List<Description> mRunTests = new ArrayList<>();
        private List<Description> mSkippedTests = new ArrayList<>();

        public List<Description> getRunTests() {
            return mRunTests;
        }

        public List<Description> getSkippedTests() {
            return mSkippedTests;
        }

        @Override
        public void testStarted(Description description) throws Exception {
            mRunTests.add(description);
        }

        @Override
        public void testFinished(Description description) throws Exception {
            Assert.assertThat(description, isIn(mRunTests));
        }

        @Override
        public void testFailure(Failure failure) throws Exception {
            Assert.fail(failure.toString());
        }

        @Override
        public void testAssumptionFailure(Failure failure) {
            Assert.fail(failure.toString());
        }

        @Override
        public void testIgnored(Description description) throws Exception {
            mSkippedTests.add(description);
        }
    }

    /**
     * Registers a fake {@link Instrumentation} so that class runners for instrumentation tests can
     * be run even in Robolectric tests.
     */
    private static class MockInstrumentationRule extends ExternalResource {
        @Override
        protected void before() throws Throwable {
            Instrumentation instrumentation = new Instrumentation() {
                @Override
                public Context getTargetContext() {
                    return RuntimeEnvironment.application;
                }
            };
            InstrumentationRegistry.registerInstance(instrumentation, new Bundle());
        }

        @Override
        protected void after() {
            InstrumentationRegistry.registerInstance(null, new Bundle());
        }
    }

    @Rule
    public TestRule mMockInstrumentationRule = new MockInstrumentationRule();

    @Rule
    public ErrorCollector mErrorCollector = new ErrorCollector();

    @Test
    public void testWithTestRunner() throws Exception {
        // TODO(bauerb): Using Mockito mock() or spy() throws a ClassCastException.
        MockRunListener runListener = new MockRunListener();
        RunNotifier runNotifier = new RunNotifier();
        runNotifier.addListener(runListener);
        new BaseJUnit4ClassRunner(ManualTest.class).run(runNotifier);

        mErrorCollector.checkThat(runListener.getRunTests(),
                contains(Description.createTestDescription(ManualTest.class, "nonManualTest")));
        mErrorCollector.checkThat(runListener.getSkippedTests(),
                contains(Description.createTestDescription(ManualTest.class, "manualTest")));
    }
}
