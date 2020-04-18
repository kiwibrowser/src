// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test;

import android.app.Activity;
import android.app.Application;
import android.app.Instrumentation;
import android.content.Context;
import android.content.pm.InstrumentationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.internal.runner.RunnerArgs;
import android.support.test.internal.runner.TestExecutor;
import android.support.test.internal.runner.TestLoader;
import android.support.test.internal.runner.TestRequest;
import android.support.test.internal.runner.TestRequestBuilder;
import android.support.test.runner.AndroidJUnitRunner;

import dalvik.system.DexFile;

import org.chromium.base.BuildConfig;
import org.chromium.base.Log;
import org.chromium.base.annotations.MainDex;
import org.chromium.base.multidex.ChromiumMultiDexInstaller;

import java.io.IOException;
import java.lang.reflect.Field;
import java.util.Enumeration;

/**
 * A custom AndroidJUnitRunner that supports multidex installer and list out test information.
 *
 * This class is the equivalent of BaseChromiumInstrumentationTestRunner in JUnit3. Please
 * beware that is this not a class runner. It is declared in test apk AndroidManifest.xml
 * <instrumentation>
 *
 * TODO(yolandyan): remove this class after all tests are converted to JUnit4. Use class runner
 * for test listing.
 */
@MainDex
public class BaseChromiumAndroidJUnitRunner extends AndroidJUnitRunner {
    private static final String LIST_ALL_TESTS_FLAG =
            "org.chromium.base.test.BaseChromiumAndroidJUnitRunner.TestList";
    private static final String LIST_TESTS_PACKAGE_FLAG =
            "org.chromium.base.test.BaseChromiumAndroidJUnitRunner.TestListPackage";
    /**
     * This flag is supported by AndroidJUnitRunner.
     *
     * See the following page for detail
     * https://developer.android.com/reference/android/support/test/runner/AndroidJUnitRunner.html
     */
    private static final String ARGUMENT_TEST_PACKAGE = "package";

    /**
     * The following arguments are corresponding to AndroidJUnitRunner command line arguments.
     * `annotation`: run with only the argument annotation
     * `notAnnotation`: run all tests except the ones with argument annotation
     * `log`: run in log only mode, do not execute tests
     *
     * For more detail, please check
     * https://developer.android.com/reference/android/support/test/runner/AndroidJUnitRunner.html
     */
    private static final String ARGUMENT_ANNOTATION = "annotation";
    private static final String ARGUMENT_NOT_ANNOTATION = "notAnnotation";
    private static final String ARGUMENT_LOG_ONLY = "log";

    private static final String TAG = "BaseJUnitRunner";

    @Override
    public Application newApplication(ClassLoader cl, String className, Context context)
            throws ClassNotFoundException, IllegalAccessException, InstantiationException {
        // The multidex support library doesn't currently support having the test apk be multidex
        // as well as the under-test apk being multidex. If MultiDex.install() is called for both,
        // then re-extraction is triggered every time due to the support library caching only a
        // single timestamp & crc.
        //
        // Attempt to install test apk multidex only if the apk-under-test is not multidex.
        // It will likely continue to be true that the two are mutually exclusive because:
        // * ProGuard enabled =>
        //      Under-test apk is single dex.
        //      Test apk duplicates under-test classes, so may need multidex.
        // * ProGuard disabled =>
        //      Under-test apk might be multidex
        //      Test apk does not duplicate classes, so does not need multidex.
        // https://crbug.com/824523
        if (!BuildConfig.IS_MULTIDEX_ENABLED) {
            ChromiumMultiDexInstaller.install(new BaseChromiumRunnerCommon.MultiDexContextWrapper(
                    getContext(), getTargetContext()));
            BaseChromiumRunnerCommon.reorderDexPathElements(cl, getContext(), getTargetContext());
        }
        return super.newApplication(cl, className, context);
    }

    /**
     * Add TestListInstrumentationRunListener when argument ask the runner to list tests info.
     *
     * The running mechanism when argument has "listAllTests" is equivalent to that of
     * {@link android.support.test.runner.AndroidJUnitRunner#onStart()} except it adds
     * only TestListInstrumentationRunListener to monitor the tests.
     */
    @Override
    public void onStart() {
        Bundle arguments = InstrumentationRegistry.getArguments();
        if (arguments != null && arguments.getString(LIST_ALL_TESTS_FLAG) != null) {
            Log.w(TAG,
                    String.format("Runner will list out tests info in JSON without running tests. "
                                    + "Arguments: %s",
                            arguments.toString()));
            listTests(); // Intentionally not calling super.onStart() to avoid additional work.
        } else {
            if (arguments != null && arguments.getString(ARGUMENT_LOG_ONLY) != null) {
                Log.e(TAG,
                        String.format("Runner will log the tests without running tests."
                                        + " If this cause a test run to fail, please report to"
                                        + " crbug.com/754015. Arguments: %s",
                                arguments.toString()));
            }
            super.onStart();
        }
    }

    // TODO(yolandyan): Move this to test harness side once this class gets removed
    private void addTestListPackage(Bundle bundle) {
        PackageManager pm = getContext().getPackageManager();
        InstrumentationInfo info;
        try {
            info = pm.getInstrumentationInfo(getComponentName(), PackageManager.GET_META_DATA);
        } catch (NameNotFoundException e) {
            Log.e(TAG, String.format("Could not find component %s", getComponentName()));
            throw new RuntimeException(e);
        }
        Bundle metaDataBundle = info.metaData;
        if (metaDataBundle != null && metaDataBundle.getString(LIST_TESTS_PACKAGE_FLAG) != null) {
            bundle.putString(
                    ARGUMENT_TEST_PACKAGE, metaDataBundle.getString(LIST_TESTS_PACKAGE_FLAG));
        }
    }

    private void listTests() {
        Bundle results = new Bundle();
        TestListInstrumentationRunListener listener = new TestListInstrumentationRunListener();
        try {
            TestExecutor.Builder executorBuilder = new TestExecutor.Builder(this);
            executorBuilder.addRunListener(listener);
            Bundle junit3Arguments = new Bundle(InstrumentationRegistry.getArguments());
            junit3Arguments.putString(ARGUMENT_NOT_ANNOTATION, "org.junit.runner.RunWith");
            addTestListPackage(junit3Arguments);
            TestRequest listJUnit3TestRequest = createListTestRequest(junit3Arguments);
            results = executorBuilder.build().execute(listJUnit3TestRequest);

            Bundle junit4Arguments = new Bundle(InstrumentationRegistry.getArguments());
            junit4Arguments.putString(ARGUMENT_ANNOTATION, "org.junit.runner.RunWith");
            addTestListPackage(junit4Arguments);

            // Do not use Log runner from android test support.
            //
            // Test logging and execution skipping is handled by BaseJUnit4ClassRunner,
            // having ARGUMENT_LOG_ONLY in argument bundle here causes AndroidJUnitRunner
            // to use its own log-only class runner instead of BaseJUnit4ClassRunner.
            junit4Arguments.remove(ARGUMENT_LOG_ONLY);

            TestRequest listJUnit4TestRequest = createListTestRequest(junit4Arguments);
            results.putAll(executorBuilder.build().execute(listJUnit4TestRequest));
            listener.saveTestsToJson(
                    InstrumentationRegistry.getArguments().getString(LIST_ALL_TESTS_FLAG));
        } catch (IOException | RuntimeException e) {
            String msg = "Fatal exception when running tests";
            Log.e(TAG, msg, e);
            // report the exception to instrumentation out
            results.putString(Instrumentation.REPORT_KEY_STREAMRESULT,
                    msg + "\n" + Log.getStackTraceString(e));
        }
        finish(Activity.RESULT_OK, results);
    }

    private TestRequest createListTestRequest(Bundle arguments) {
        RunnerArgs runnerArgs =
                new RunnerArgs.Builder().fromManifest(this).fromBundle(arguments).build();
        TestRequestBuilder builder = new IncrementalInstallTestRequestBuilder(this, arguments);
        builder.addFromRunnerArgs(runnerArgs);
        builder.addApkToScan(getContext().getPackageCodePath());
        return builder.build();
    }

    static boolean shouldListTests(Bundle arguments) {
        return arguments != null && arguments.getString(LIST_ALL_TESTS_FLAG) != null;
    }

    /**
     * Wraps TestRequestBuilder to make it work with incremental install.
     */
    private static class IncrementalInstallTestRequestBuilder extends TestRequestBuilder {
        boolean mHasClassList;

        public IncrementalInstallTestRequestBuilder(Instrumentation instr, Bundle bundle) {
            super(instr, bundle);
        }

        @Override
        public TestRequestBuilder addTestClass(String className) {
            mHasClassList = true;
            return super.addTestClass(className);
        }

        @Override
        public TestRequestBuilder addTestMethod(String testClassName, String testMethodName) {
            mHasClassList = true;
            return super.addTestMethod(testClassName, testMethodName);
        }

        @Override
        public TestRequest build() {
            // See crbug://841695. TestLoader.isTestClass is incorrectly deciding that
            // InstrumentationTestSuite is a test class.
            removeTestClass("android.test.InstrumentationTestSuite");
            // If a test class was requested, then no need to iterate class loader.
            if (mHasClassList) {
                return super.build();
            }
            maybeScanIncrementalClasspath();
            return super.build();
        }

        private void maybeScanIncrementalClasspath() {
            DexFile[] incrementalJars = null;
            try {
                Class<?> bootstrapClass =
                        Class.forName("org.chromium.incrementalinstall.BootstrapApplication");
                incrementalJars =
                        (DexFile[]) bootstrapClass.getDeclaredField("sIncrementalDexFiles")
                                .get(null);
            } catch (Exception e) {
                // Not an incremental apk.
            }
            if (incrementalJars != null) {
                // builder.addApkToScan uses new DexFile(path) under the hood, which on Dalvik OS's
                // assumes that the optimized dex is in the default location (crashes).
                // Perform our own dex file scanning instead as a workaround.
                addTestClasses(incrementalJars, this);
            }
        }

        private boolean startsWithAny(String str, String[] prefixes) {
            for (String prefix : prefixes) {
                if (str.startsWith(prefix)) {
                    return true;
                }
            }
            return false;
        }

        private void addTestClasses(DexFile[] dexFiles, TestRequestBuilder builder) {
            Log.i(TAG, "Scanning incremental classpath.");
            String[] excludedPrefixes;
            try {
                Field excludedPackagesField =
                        TestRequestBuilder.class.getDeclaredField("DEFAULT_EXCLUDED_PACKAGES");
                excludedPackagesField.setAccessible(true);
                excludedPrefixes = (String[]) excludedPackagesField.get(null);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }

            // Mirror TestRequestBuilder.getClassNamesFromClassPath().
            TestLoader loader = new TestLoader();
            for (DexFile dexFile : dexFiles) {
                Enumeration<String> classNames = dexFile.entries();
                while (classNames.hasMoreElements()) {
                    String className = classNames.nextElement();
                    if (!className.contains("$") && !startsWithAny(className, excludedPrefixes)
                            && loader.loadIfTest(className) != null) {
                        addTestClass(className);
                    }
                }
            }
        }
    }
}
