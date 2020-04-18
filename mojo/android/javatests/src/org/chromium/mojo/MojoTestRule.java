// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo;

import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;

/**
 * Base class to test mojo. Setup the environment.
 */
@JNINamespace("mojo::android")
public class MojoTestRule implements TestRule {

    private long mTestEnvironmentPointer;

    public void ruleSetUp() throws Exception {
        LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_BROWSER);
        nativeInit();
        mTestEnvironmentPointer = nativeSetupTestEnvironment();
    }

    public void ruleTearDown() throws Exception {
        nativeTearDownTestEnvironment(mTestEnvironmentPointer);
    }


    /**
     * Runs the run loop for the given time.
     */
    public void runLoop(long timeoutMS) {
        nativeRunLoop(timeoutMS);
    }

    /**
     * Runs the run loop until no handle or task are immediately available.
     */
    public void runLoopUntilIdle() {
        nativeRunLoop(0);
    }

    @Override
    public Statement apply(final Statement base, Description desc) {
        return new Statement() {
            @Override
            public void evaluate() throws Throwable {
                ruleSetUp();
                base.evaluate();
                ruleTearDown();
            }
        };
    }

    private native void nativeInit();

    private native long nativeSetupTestEnvironment();

    private native void nativeTearDownTestEnvironment(long testEnvironment);

    private native void nativeRunLoop(long timeoutMS);

}
