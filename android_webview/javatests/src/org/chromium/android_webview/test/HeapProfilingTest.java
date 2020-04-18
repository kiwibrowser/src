// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.components.heap_profiling.HeapProfilingTestShim;

/**
 * Tests suite for heap profiling.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class HeapProfilingTest {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    @Before
    public void setUp() throws Exception {}

    @Test
    @MediumTest
    @CommandLineFlags.Add({"memlog=browser", "memlog-stack-mode=native-include-thread-names"})
    @SkipSingleProcessTests
    public void testModeBrowser() throws Exception {
        HeapProfilingTestShim shim = new HeapProfilingTestShim();
        Assert.assertTrue(
                shim.runTestForMode("browser", false, "native-include-thread-names", false, false));
    }

    @Test
    @MediumTest
    @SkipSingleProcessTests
    public void testModeBrowserDynamicPseudo() throws Exception {
        HeapProfilingTestShim shim = new HeapProfilingTestShim();
        Assert.assertTrue(shim.runTestForMode("browser", true, "pseudo", false, false));
    }

    @Test
    @MediumTest
    @SkipSingleProcessTests
    public void testModeBrowserDynamicPseudoSampleEverything() throws Exception {
        HeapProfilingTestShim shim = new HeapProfilingTestShim();
        Assert.assertTrue(shim.runTestForMode("browser", true, "pseudo", true, true));
    }

    @Test
    @MediumTest
    @SkipSingleProcessTests
    public void testModeBrowserDynamicPseudoSamplePartial() throws Exception {
        HeapProfilingTestShim shim = new HeapProfilingTestShim();
        Assert.assertTrue(shim.runTestForMode("browser", true, "pseudo", true, false));
    }
}
