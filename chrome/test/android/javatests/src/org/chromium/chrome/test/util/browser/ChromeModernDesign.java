// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNotNull;

import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.base.StrictModeContext;
import org.chromium.base.test.util.AnnotationRule;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.ui.test.util.UiRestriction;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Utility annotation and rule to enable or disable ChromeModernDesign. Handles setting and
 * resetting the feature flag and the preference.
 *
 * @see ChromeModernDesign.Processor
 * @see FeatureUtilities#isChromeModernDesignEnabled()
 */
public interface ChromeModernDesign {
    @Retention(RetentionPolicy.RUNTIME)
    @Target({ElementType.METHOD, ElementType.TYPE})
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    @Features.EnableFeatures(ChromeFeatureList.CHROME_MODERN_DESIGN)
    @interface Enable {}

    @Retention(RetentionPolicy.RUNTIME)
    @Target({ElementType.METHOD, ElementType.TYPE})
    @Features.DisableFeatures(ChromeFeatureList.CHROME_MODERN_DESIGN)
    @interface Disable {}

    /**
     * Rule to handle setting and resetting the cached feature state for ChromeModernDesign. Can be
     * used by explicitly calling methods ({@link #setPrefs(boolean)} and {@link #clearTestState()})
     * or by using the {@link ChromeModernDesign.Enable} and {@link ChromeModernDesign.Disable}
     * annotations on tests.
     */
    class Processor extends AnnotationRule {
        private Boolean mOldState;

        public Processor() {
            super(ChromeModernDesign.Enable.class, ChromeModernDesign.Disable.class);
        }

        @Override
        public Statement apply(Statement base, Description description) {
            Statement wrappedStatement = super.apply(base, description);
            return getAnnotations().isEmpty() ? base : wrappedStatement;
        }

        @Override
        protected void before() throws Throwable {
            boolean featureEnabled = getClosestAnnotation() instanceof ChromeModernDesign.Enable;
            setPrefs(featureEnabled);
        }

        @Override
        protected void after() {
            clearTestState();
        }

        public void setPrefs(boolean enabled) {
            // Chrome relies on a shared preference to determine if the ChromeModernDesign feature
            // is enabled during start up, so we need to manually set the preference to enable
            // ChromeModernDesign before starting Chrome.
            ChromePreferenceManager prefsManager = ChromePreferenceManager.getInstance();
            try (StrictModeContext unused = StrictModeContext.allowDiskReads()) {
                mOldState = prefsManager.isChromeModernDesignEnabled();
            }

            FeatureUtilities.resetChromeModernDesignEnabledForTests();

            // The native library should not be enabled yet, so we set the preference here and
            // cache it by checking for the feature. Ideally we instead would call
            // FeatureUtilities.cacheChromeModernDesignEnabled()
            prefsManager.setChromeModernDesignEnabled(enabled);
            assertEquals(enabled, FeatureUtilities.isChromeModernDesignEnabled());
        }

        public void clearTestState() {
            assertNotNull(mOldState);
            ChromePreferenceManager.getInstance().setChromeModernDesignEnabled(mOldState);
        }
    }
}
