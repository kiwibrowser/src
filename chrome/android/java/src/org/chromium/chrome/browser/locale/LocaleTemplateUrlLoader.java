// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.locale;

/**
 * A loader class for changes of template url in a given special locale. This is a JNI bridge and
 * it owns the native object. Make sure to call destroy() after this object is not used anymore.
 */
public class LocaleTemplateUrlLoader {
    private final String mLocaleId;
    private long mNativeLocaleTemplateUrlLoader;
    private boolean mAddedToService;

    /**
     * Creates a {@link LocaleTemplateUrlLoader} that handles changes for the given locale.
     * @param localeId Country id of the locale. Should be 2 characters long.
     */
    public LocaleTemplateUrlLoader(String localeId) {
        assert localeId.length() == 2;
        mLocaleId = localeId;
        mNativeLocaleTemplateUrlLoader = nativeInit(localeId);
    }

    /**
     * This *must* be called after the {@link LocaleTemplateUrlLoader} is not used anymore.
     */
    public void destroy() {
        assert mNativeLocaleTemplateUrlLoader != 0;
        nativeDestroy(mNativeLocaleTemplateUrlLoader);
        mNativeLocaleTemplateUrlLoader = 0;
    }

    /**
     * Loads the template urls for this locale, and adds it to template url service. If the device
     * was initialized in the given special locale, no-op here.
     * @return Whether loading is needed.
     */
    public boolean loadTemplateUrls() {
        assert mNativeLocaleTemplateUrlLoader != 0;
        // If the locale is the same as the one set at install time, there is no need to load the
        // search engines, as they are already cached in the template url service.
        mAddedToService = nativeLoadTemplateUrls(mNativeLocaleTemplateUrlLoader);
        return mAddedToService;
    }

    /**
     * Removes the template urls that was added by {@link #loadTemplateUrls()}. No-op if
     * {@link #loadTemplateUrls()} returned false.
     */
    public void removeTemplateUrls() {
        assert mNativeLocaleTemplateUrlLoader != 0;
        if (mAddedToService) nativeRemoveTemplateUrls(mNativeLocaleTemplateUrlLoader);
    }

    /**
     * Overrides the default search provider in special locale.
     */
    public void overrideDefaultSearchProvider() {
        assert mNativeLocaleTemplateUrlLoader != 0;
        nativeOverrideDefaultSearchProvider(mNativeLocaleTemplateUrlLoader);
    }

    /**
     * Sets the default search provider back to Google.
     */
    public void setGoogleAsDefaultSearch() {
        assert mNativeLocaleTemplateUrlLoader != 0;
        nativeSetGoogleAsDefaultSearch(mNativeLocaleTemplateUrlLoader);
    }

    private static native long nativeInit(String localeId);
    private static native void nativeDestroy(long nativeLocaleTemplateUrlLoader);
    private static native boolean nativeLoadTemplateUrls(long nativeLocaleTemplateUrlLoader);
    private static native void nativeRemoveTemplateUrls(long nativeLocaleTemplateUrlLoader);
    private static native void nativeOverrideDefaultSearchProvider(
            long nativeLocaleTemplateUrlLoader);
    private static native void nativeSetGoogleAsDefaultSearch(long nativeLocaleTemplateUrlLoader);
}
