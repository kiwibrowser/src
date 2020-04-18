// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.dom_distiller;

import android.support.test.InstrumentationRegistry;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.SmallTest;
import android.support.test.rule.UiThreadTestRule;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.components.dom_distiller.core.DistilledPagePrefs;
import org.chromium.components.dom_distiller.core.DomDistillerService;
import org.chromium.components.dom_distiller.core.FontFamily;
import org.chromium.components.dom_distiller.core.Theme;
import org.chromium.content.browser.test.util.UiUtils;

/**
 * Test class for {@link DistilledPagePrefs}.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class DistilledPagePrefsTest {
    @Rule
    public final RuleChain mChain =
            RuleChain.outerRule(new ChromeBrowserTestRule()).around(new UiThreadTestRule());

    private DistilledPagePrefs mDistilledPagePrefs;

    private static final double EPSILON = 1e-5;

    @Before
    public void setUp() throws Exception {
        getDistilledPagePrefs();
    }

    private void getDistilledPagePrefs() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                DomDistillerService domDistillerService = DomDistillerServiceFactory
                        .getForProfile(Profile.getLastUsedProfile());
                mDistilledPagePrefs = domDistillerService.getDistilledPagePrefs();
            }
        });
    }

    @Test
    @SmallTest
    @UiThreadTest
    @Feature({"DomDistiller"})
    public void testGetAndSetTheme() throws Throwable {
        // Check the default theme.
        Assert.assertEquals(Theme.LIGHT, mDistilledPagePrefs.getTheme());
        // Check that theme can be correctly set.
        setTheme(Theme.DARK);
        Assert.assertEquals(Theme.DARK, mDistilledPagePrefs.getTheme());
        setTheme(Theme.LIGHT);
        Assert.assertEquals(Theme.LIGHT, mDistilledPagePrefs.getTheme());
        setTheme(Theme.SEPIA);
        Assert.assertEquals(Theme.SEPIA, mDistilledPagePrefs.getTheme());
    }

    /*
    @SmallTest
    @Feature({"DomDistiller"})
    */
    @Test
    @DisabledTest(message = "crbug.com/458196")
    public void testSingleObserverTheme() throws InterruptedException {
        TestingObserver testObserver = new TestingObserver();
        mDistilledPagePrefs.addObserver(testObserver);

        setTheme(Theme.DARK);
        // Assumes that callback does not occur immediately.
        Assert.assertNull(testObserver.getTheme());
        UiUtils.settleDownUI(InstrumentationRegistry.getInstrumentation());
        // Check that testObserver's theme has been updated,
        Assert.assertEquals(Theme.DARK, testObserver.getTheme());
        mDistilledPagePrefs.removeObserver(testObserver);
    }

    /*
    @SmallTest
    @Feature({"DomDistiller"})
    */
    @Test
    @DisabledTest(message = "crbug.com/458196")
    public void testMultipleObserversTheme() throws InterruptedException {
        TestingObserver testObserverOne = new TestingObserver();
        mDistilledPagePrefs.addObserver(testObserverOne);
        TestingObserver testObserverTwo = new TestingObserver();
        mDistilledPagePrefs.addObserver(testObserverTwo);

        setTheme(Theme.SEPIA);
        UiUtils.settleDownUI(InstrumentationRegistry.getInstrumentation());
        Assert.assertEquals(Theme.SEPIA, testObserverOne.getTheme());
        Assert.assertEquals(Theme.SEPIA, testObserverTwo.getTheme());
        mDistilledPagePrefs.removeObserver(testObserverOne);

        setTheme(Theme.DARK);
        UiUtils.settleDownUI(InstrumentationRegistry.getInstrumentation());
        // Check that testObserverOne's theme is not changed but testObserverTwo's is.
        Assert.assertEquals(Theme.SEPIA, testObserverOne.getTheme());
        Assert.assertEquals(Theme.DARK, testObserverTwo.getTheme());
        mDistilledPagePrefs.removeObserver(testObserverTwo);
    }

    @Test
    @SmallTest
    @UiThreadTest
    @Feature({"DomDistiller"})
    public void testGetAndSetFontFamily() throws Throwable {
        // Check the default font family.
        Assert.assertEquals(FontFamily.SANS_SERIF, mDistilledPagePrefs.getFontFamily());
        // Check that font family can be correctly set.
        setFontFamily(FontFamily.SERIF);
        Assert.assertEquals(FontFamily.SERIF, mDistilledPagePrefs.getFontFamily());
    }

    /*
    @SmallTest
    @Feature({"DomDistiller"})
    crbug.com/458196
    */
    @Test
    @DisabledTest
    public void testSingleObserverFontFamily() throws InterruptedException {
        TestingObserver testObserver = new TestingObserver();
        mDistilledPagePrefs.addObserver(testObserver);

        setFontFamily(FontFamily.SERIF);
        // Assumes that callback does not occur immediately.
        Assert.assertNull(testObserver.getFontFamily());
        UiUtils.settleDownUI(InstrumentationRegistry.getInstrumentation());
        // Check that testObserver's font family has been updated,
        Assert.assertEquals(FontFamily.SERIF, testObserver.getFontFamily());
        mDistilledPagePrefs.removeObserver(testObserver);
    }

    /*
    @SmallTest
    @Feature({"DomDistiller"})
    crbug.com/458196
    */
    @Test
    @DisabledTest
    public void testMultipleObserversFontFamily() throws InterruptedException {
        TestingObserver testObserverOne = new TestingObserver();
        mDistilledPagePrefs.addObserver(testObserverOne);
        TestingObserver testObserverTwo = new TestingObserver();
        mDistilledPagePrefs.addObserver(testObserverTwo);

        setFontFamily(FontFamily.MONOSPACE);
        UiUtils.settleDownUI(InstrumentationRegistry.getInstrumentation());
        Assert.assertEquals(FontFamily.MONOSPACE, testObserverOne.getFontFamily());
        Assert.assertEquals(FontFamily.MONOSPACE, testObserverTwo.getFontFamily());
        mDistilledPagePrefs.removeObserver(testObserverOne);

        setFontFamily(FontFamily.SERIF);
        UiUtils.settleDownUI(InstrumentationRegistry.getInstrumentation());
        // Check that testObserverOne's font family is not changed but testObserverTwo's is.
        Assert.assertEquals(FontFamily.MONOSPACE, testObserverOne.getFontFamily());
        Assert.assertEquals(FontFamily.SERIF, testObserverTwo.getFontFamily());
        mDistilledPagePrefs.removeObserver(testObserverTwo);
    }

    @Test
    @SmallTest
    @UiThreadTest
    @Feature({"DomDistiller"})
    public void testGetAndSetFontScaling() throws Throwable {
        // Check the default font scaling.
        Assert.assertEquals(1.0, mDistilledPagePrefs.getFontScaling(), EPSILON);
        // Check that font scaling can be correctly set.
        setFontScaling(1.2f);
        Assert.assertEquals(1.2, mDistilledPagePrefs.getFontScaling(), EPSILON);
    }

    /*
    @SmallTest
    @Feature({"DomDistiller"})
    crbug.com/458196
    */
    @Test
    @DisabledTest
    public void testSingleObserverFontScaling() throws InterruptedException {
        TestingObserver testObserver = new TestingObserver();
        mDistilledPagePrefs.addObserver(testObserver);

        setFontScaling(1.1f);
        // Assumes that callback does not occur immediately.
        Assert.assertNull(testObserver.getFontScaling());
        UiUtils.settleDownUI(InstrumentationRegistry.getInstrumentation());
        // Check that testObserver's font scaling has been updated,
        Assert.assertEquals(1.1, testObserver.getFontScaling(), EPSILON);
        mDistilledPagePrefs.removeObserver(testObserver);
    }

    /*
    @SmallTest
    @Feature({"DomDistiller"})
    crbug.com/458196
    */
    @Test
    @DisabledTest
    public void testMultipleObserversFontScaling() throws InterruptedException {
        TestingObserver testObserverOne = new TestingObserver();
        mDistilledPagePrefs.addObserver(testObserverOne);
        TestingObserver testObserverTwo = new TestingObserver();
        mDistilledPagePrefs.addObserver(testObserverTwo);

        setFontScaling(1.3f);
        UiUtils.settleDownUI(InstrumentationRegistry.getInstrumentation());
        Assert.assertEquals(1.3, testObserverOne.getFontScaling(), EPSILON);
        Assert.assertEquals(1.3, testObserverTwo.getFontScaling(), EPSILON);
        mDistilledPagePrefs.removeObserver(testObserverOne);

        setFontScaling(0.9f);
        UiUtils.settleDownUI(InstrumentationRegistry.getInstrumentation());
        // Check that testObserverOne's font scaling is not changed but testObserverTwo's is.
        Assert.assertEquals(1.3, testObserverOne.getFontScaling(), EPSILON);
        Assert.assertEquals(0.9, testObserverTwo.getFontScaling(), EPSILON);
        mDistilledPagePrefs.removeObserver(testObserverTwo);
    }

    @Test
    @SmallTest
    @Feature({"DomDistiller"})
    public void testRepeatedAddAndDeleteObserver() throws InterruptedException {
        TestingObserver test = new TestingObserver();

        // Should successfully add the observer the first time.
        Assert.assertTrue(mDistilledPagePrefs.addObserver(test));
        // Observer cannot be added again, should return false.
        Assert.assertFalse(mDistilledPagePrefs.addObserver(test));

        // Delete the observer the first time.
        Assert.assertTrue(mDistilledPagePrefs.removeObserver(test));
        // Observer cannot be deleted again, should return false.
        Assert.assertFalse(mDistilledPagePrefs.removeObserver(test));
    }

    private static class TestingObserver implements DistilledPagePrefs.Observer {
        private FontFamily mFontFamily;
        private Theme mTheme;
        private float mFontScaling;

        public TestingObserver() {}

        public FontFamily getFontFamily() {
            return mFontFamily;
        }

        @Override
        public void onChangeFontFamily(FontFamily font) {
            mFontFamily = font;
        }

        public Theme getTheme() {
            return mTheme;
        }

        @Override
        public void onChangeTheme(Theme theme) {
            mTheme = theme;
        }

        public float getFontScaling() {
            return mFontScaling;
        }

        @Override
        public void onChangeFontScaling(float scaling) {
            mFontScaling = scaling;
        }
    }

    private void setFontFamily(final FontFamily font) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mDistilledPagePrefs.setFontFamily(font);
            }
        });
    }

    private void setTheme(final Theme theme) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mDistilledPagePrefs.setTheme(theme);
            }
        });
    }

    private void setFontScaling(final float scaling) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mDistilledPagePrefs.setFontScaling(scaling);
            }
        });
    }
}
