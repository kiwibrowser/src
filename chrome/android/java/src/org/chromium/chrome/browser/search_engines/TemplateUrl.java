// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.search_engines;

import org.chromium.base.annotations.CalledByNative;

import java.util.Locale;

/**
 * Represents object of a search engine. It only caches the native pointer of TemplateURL object
 * from native side. Any class uses this need to register a {@link TemplateUrlServiceObserver} on
 * {@link TemplatUrlService} to listen the native changes in case the native pointer is destroyed.
 */
public class TemplateUrl {
    private final long mTemplateUrlPtr;

    @CalledByNative
    private static TemplateUrl create(long templateUrlPtr) {
        return new TemplateUrl(templateUrlPtr);
    }

    protected TemplateUrl(long templateUrlPtr) {
        mTemplateUrlPtr = templateUrlPtr;
    }

    /**
     * @return The name of the search engine.
     */
    public String getShortName() {
        return nativeGetShortName(mTemplateUrlPtr);
    }

    /**
     * @return The prepopulated id of the search engine. For predefined engines, this field is a
     *         non-zero, for custom search engines, it will return 0.
     */
    public int getPrepopulatedId() {
        return nativeGetPrepopulatedId(mTemplateUrlPtr);
    }

    /**
     * @return Whether a search engine is prepopulated or created by policy.
     */
    public boolean getIsPrepopulated() {
        return nativeIsPrepopulatedOrCreatedByPolicy(mTemplateUrlPtr);
    }

    /**
     * @return The keyword of the search engine.
     */
    public String getKeyword() {
        return nativeGetKeyword(mTemplateUrlPtr);
    }

    /**
     * @return The last time used this search engine. If a search engine hasn't been used, it will
     *         return 0.
     */
    public long getLastVisitedTime() {
        return nativeGetLastVisitedTime(mTemplateUrlPtr);
    }

    @Override
    public boolean equals(Object other) {
        if (!(other instanceof TemplateUrl)) return false;
        TemplateUrl otherTemplateUrl = (TemplateUrl) other;
        return mTemplateUrlPtr == otherTemplateUrl.mTemplateUrlPtr;
    }

    @Override
    public String toString() {
        return String.format(Locale.US,
                "TemplateURL -- keyword: %s, short name: %s, "
                        + "prepopulated: %b",
                getKeyword(), getShortName(), getIsPrepopulated());
    }

    private static native String nativeGetShortName(long templateUrlPtr);
    private static native String nativeGetKeyword(long templateUrlPtr);
    private static native boolean nativeIsPrepopulatedOrCreatedByPolicy(long templateUrlPtr);
    private static native long nativeGetLastVisitedTime(long templateUrlPtr);
    private static native int nativeGetPrepopulatedId(long templateUrlPtr);
}
