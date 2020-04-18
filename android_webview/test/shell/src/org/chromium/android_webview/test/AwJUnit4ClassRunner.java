// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import org.junit.runner.notification.RunNotifier;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;

import org.chromium.android_webview.AwSwitches;
import org.chromium.base.CollectionUtil;
import org.chromium.base.CommandLine;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.BaseTestResult.PreTestHook;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.parameter.SkipCommandLineParameterization;
import org.chromium.policy.test.annotations.Policies;

import java.util.ArrayList;
import java.util.List;

/**
 * A custom runner for //android_webview instrumentation tests. WebView only runs on KitKat and
 * later, so make sure no one attempts to run the tests on earlier OS releases.
 * By default, all tests run both in single-process mode, and with sandboxed
 * renderer. If a test doesn't yet work with sandboxed renderer, an entire
 * class, or an individual test method can be marked for single-process testing
 * only by adding @SkipCommandLineParameterization to the test.
 *
 * Single process tests can be skipped by adding @SkipSingleProcessTests.
 */
public final class AwJUnit4ClassRunner extends BaseJUnit4ClassRunner {
    /**
     * Create an AwJUnit4ClassRunner to run {@code klass} and initialize values
     *
     * @param klass Test class to run
     * @throws InitializationError if the test class is malformed
     */
    public AwJUnit4ClassRunner(Class<?> klass) throws InitializationError {
        super(klass, null, defaultPreTestHooks());
    }

    private static List<PreTestHook> defaultPreTestHooks() {
        return CollectionUtil.newArrayList(Policies.getRegistrationHook());
    }

    @Override
    protected List<FrameworkMethod> getChildren() {
        List<FrameworkMethod> result = new ArrayList<>();
        for (FrameworkMethod method : computeTestMethods()) {
            if (method.getAnnotation(SkipCommandLineParameterization.class) == null) {
                result.add(new WebViewMultiProcessFrameworkMethod(method));
            }
            if (method.getAnnotation(SkipSingleProcessTests.class) == null) {
                result.add(method);
            }
        }
        return result;
    }

    @Override
    protected void runChild(FrameworkMethod method, RunNotifier notifier) {
        CommandLineFlags.setUp(method.getMethod());
        if (method instanceof WebViewMultiProcessFrameworkMethod) {
            CommandLine.getInstance().appendSwitch(AwSwitches.WEBVIEW_SANDBOXED_RENDERER);
        }
        super.runChild(method, notifier);
    }

    /**
     * Custom FrameworkMethod class indicate this test method will run in multiprocess mode.
     *
     * The clas also add "__multiprocess_mode" postfix to the test name.
     */
    private static class WebViewMultiProcessFrameworkMethod extends FrameworkMethod {
        public WebViewMultiProcessFrameworkMethod(FrameworkMethod method) {
            super(method.getMethod());
        }

        @Override
        public String getName() {
            return super.getName() + "__multiprocess_mode";
        }

        @Override
        public boolean equals(Object obj) {
            if (obj instanceof WebViewMultiProcessFrameworkMethod) {
                WebViewMultiProcessFrameworkMethod method =
                        (WebViewMultiProcessFrameworkMethod) obj;
                return super.equals(obj) && method.getName().equals(getName());
            }
            return false;
        }

        @Override
        public int hashCode() {
            int result = 17;
            result = 31 * result + super.hashCode();
            result = 31 * result + getName().hashCode();
            return result;
        }
    }
}
