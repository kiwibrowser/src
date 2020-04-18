// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.search_engines;

import android.support.annotation.Nullable;

import org.chromium.base.ObserverList;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;

import java.util.ArrayList;
import java.util.List;

/**
 * Android wrapper of the TemplateUrlService which provides access from the Java
 * layer.
 *
 * Only usable from the UI thread as it's primary purpose is for supporting the Android
 * preferences UI.
 *
 * See components/search_engines/template_url_service.h for more details.
 */
public class TemplateUrlService {

    /**
     * This listener will be notified when template url service is done loading.
     */
    public interface LoadListener {
        void onTemplateUrlServiceLoaded();
    }

    /**
     * Observer to be notified whenever the set of TemplateURLs are modified.
     */
    public interface TemplateUrlServiceObserver {
        /**
         * Notification that the template url model has changed in some way.
         */
        void onTemplateURLServiceChanged();
    }

    private static TemplateUrlService sService;

    public static TemplateUrlService getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sService == null) {
            sService = new TemplateUrlService();
        }
        return sService;
    }

    private final long mNativeTemplateUrlServiceAndroid;
    private final ObserverList<LoadListener> mLoadListeners = new ObserverList<LoadListener>();
    private final ObserverList<TemplateUrlServiceObserver> mObservers =
            new ObserverList<TemplateUrlServiceObserver>();

    private TemplateUrlService() {
        // Note that this technically leaks the native object, however, TemlateUrlService
        // is a singleton that lives forever and there's no clean shutdown of Chrome on Android
        mNativeTemplateUrlServiceAndroid = nativeInit();
    }

    public boolean isLoaded() {
        ThreadUtils.assertOnUiThread();
        return nativeIsLoaded(mNativeTemplateUrlServiceAndroid);
    }

    public void load() {
        ThreadUtils.assertOnUiThread();
        nativeLoad(mNativeTemplateUrlServiceAndroid);
    }

    /**
     * Ensure the TemplateUrlService is loaded before running the specified action.  If the service
     * is already loaded, then run the action immediately.
     * <p>
     * Because this can introduce an arbitrary delay in the action being executed, ensure the state
     * is still valid in the action before interacting with anything that might no longer be
     * available (i.e. an Activity that has since been destroyed).
     *
     * @param action The action to be run.
     */
    public void runWhenLoaded(final Runnable action) {
        if (isLoaded()) {
            action.run();
        } else {
            registerLoadListener(new LoadListener() {
                @Override
                public void onTemplateUrlServiceLoaded() {
                    unregisterLoadListener(this);
                    action.run();
                }
            });
            load();
        }
    }

    /**
     * Returns a list of the all available search engines.
     */
    public List<TemplateUrl> getTemplateUrls() {
        ThreadUtils.assertOnUiThread();
        List<TemplateUrl> templateUrls = new ArrayList<>();
        nativeGetTemplateUrls(mNativeTemplateUrlServiceAndroid, templateUrls);
        return templateUrls;
    }

    /**
     * Called from native to populate the list of all available search engines.
     * @param templateUrls The list of {@link TemplateUrl} to be added.
     * @param templateUrl The {@link TemplateUrl} would add to the list.
     */
    @CalledByNative
    private static void addTemplateUrlToList(
            List<TemplateUrl> templateUrls, TemplateUrl templateUrl) {
        templateUrls.add(templateUrl);
    }

    /**
     * Called from native when template URL service is done loading.
     */
    @CalledByNative
    private void templateUrlServiceLoaded() {
        ThreadUtils.assertOnUiThread();
        for (LoadListener listener : mLoadListeners) {
            listener.onTemplateUrlServiceLoaded();
        }
    }

    @CalledByNative
    private void onTemplateURLServiceChanged() {
        for (TemplateUrlServiceObserver observer : mObservers) {
            observer.onTemplateURLServiceChanged();
        }
    }

    /**
     * @return {@link TemplateUrl} for the default search engine.  This can
     *         be null if DSEs are disabled entirely by administrators.
     */
    public @Nullable TemplateUrl getDefaultSearchEngineTemplateUrl() {
        if (!isLoaded()) return null;
        return nativeGetDefaultSearchEngine(mNativeTemplateUrlServiceAndroid);
    }

    public void setSearchEngine(String selectedKeyword) {
        ThreadUtils.assertOnUiThread();
        nativeSetUserSelectedDefaultSearchProvider(
                mNativeTemplateUrlServiceAndroid, selectedKeyword);
    }

    /**
     * @return Whether the default search engine is managed and controlled by policy.  If true, the
     *         DSE can not be modified by the user.
     */
    public boolean isDefaultSearchManaged() {
        return nativeIsDefaultSearchManaged(mNativeTemplateUrlServiceAndroid);
    }

    /**
     * @return Whether or not the default search engine has search by image support.
     */
    public boolean isSearchByImageAvailable() {
        ThreadUtils.assertOnUiThread();
        return nativeIsSearchByImageAvailable(mNativeTemplateUrlServiceAndroid);
    }

    /**
     * @return Whether the default configured search engine is for a Google property.
     */
    public boolean isDefaultSearchEngineGoogle() {
        return nativeIsDefaultSearchEngineGoogle(mNativeTemplateUrlServiceAndroid);
    }

    public boolean doesDefaultSearchEngineHaveLogo() {
        return nativeDoesDefaultSearchEngineHaveLogo(mNativeTemplateUrlServiceAndroid);
    }

    /**
     * Checks whether a search result page is from a default search provider.
     * @param url The url for the search result page.
     * @return Whether the search result page with the given url from the default search provider.
     */
    public boolean isSearchResultsPageFromDefaultSearchProvider(String url) {
        ThreadUtils.assertOnUiThread();
        return nativeIsSearchResultsPageFromDefaultSearchProvider(
                mNativeTemplateUrlServiceAndroid, url);
    }

    /**
     * Registers a listener for the callback that indicates that the
     * TemplateURLService has loaded.
     */
    public void registerLoadListener(final LoadListener listener) {
        ThreadUtils.assertOnUiThread();
        boolean added = mLoadListeners.addObserver(listener);
        assert added;

        // If the load has already been completed, post a load complete to the observer.  Done
        // as an asynchronous call to keep the client code predictable in the loaded/unloaded state.
        if (isLoaded()) {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (!mLoadListeners.hasObserver(listener)) return;

                    listener.onTemplateUrlServiceLoaded();
                }
            });
        }
    }

    /**
     * Unregisters a listener for the callback that indicates that the
     * TemplateURLService has loaded.
     */
    public void unregisterLoadListener(LoadListener listener) {
        ThreadUtils.assertOnUiThread();
        boolean removed = mLoadListeners.removeObserver(listener);
        assert removed;
    }

    /**
     * Adds an observer to be notified on changes to the template URLs.
     * @param observer The observer to be added.
     */
    public void addObserver(TemplateUrlServiceObserver observer) {
        mObservers.addObserver(observer);
    }

    /**
     * Removes an observer for changes to the template URLs.
     * @param observer The observer to be removed.
     */
    public void removeObserver(TemplateUrlServiceObserver observer) {
        mObservers.removeObserver(observer);
    }

    /**
     * Finds the default search engine for the default provider and returns the url query
     * {@link String} for {@code query}.
     * @param query The {@link String} that represents the text query the search url should
     *              represent.
     * @return      A {@link String} that contains the url of the default search engine with
     *              {@code query} inserted as the search parameter.
     */
    public String getUrlForSearchQuery(String query) {
        return nativeGetUrlForSearchQuery(mNativeTemplateUrlServiceAndroid, query);
    }

    /**
     * Finds the default search engine for the default provider and returns the url query
     * {@link String} for {@code query} with voice input source param set.
     * @param query The {@link String} that represents the text query the search url should
     *              represent.
     * @return      A {@link String} that contains the url of the default search engine with
     *              {@code query} inserted as the search parameter and voice input source param set.
     */
    public String getUrlForVoiceSearchQuery(String query) {
        return nativeGetUrlForVoiceSearchQuery(mNativeTemplateUrlServiceAndroid, query);
    }

    /**
     * Replaces the search terms from {@code query} in {@code url}.
     * @param query The {@link String} that represents the text query that should replace the
     *              existing query in {@code url}.
     * @param url   The {@link String} that contains the search url with another search query that
     *              will be replaced with {@code query}.
     * @return      A new version of {@code url} with the search term replaced with {@code query}.
     */
    public String replaceSearchTermsInUrl(String query, String url) {
        return nativeReplaceSearchTermsInUrl(mNativeTemplateUrlServiceAndroid, query, url);
    }

    /**
     * Finds the default search engine for the default provider and returns the url query
     * {@link String} for {@code query} with the contextual search version param set.
     * @param query The search term to use as the main query in the returned search url.
     * @param alternateTerm The alternate search term to use as an alternate suggestion.
     * @param shouldPrefetch Whether the returned url should include a prefetch parameter.
     * @param protocolVersion The version of the Contextual Search API protocol to use.
     * @return      A {@link String} that contains the url of the default search engine with
     *              {@code query} and {@code alternateTerm} inserted as parameters and contextual
     *              search and prefetch parameters conditionally set.
     */
    public String getUrlForContextualSearchQuery(
            String query, String alternateTerm, boolean shouldPrefetch, String protocolVersion) {
        return nativeGetUrlForContextualSearchQuery(mNativeTemplateUrlServiceAndroid, query,
                alternateTerm, shouldPrefetch, protocolVersion);
    }

    /**
     * Finds the URL for the search engine for the given keyword.
     * @param keyword The templateUrl keyword to look up.
     * @return      A {@link String} that contains the url of the specified search engine.
     */
    public String getSearchEngineUrlFromTemplateUrl(String keyword) {
        return nativeGetSearchEngineUrlFromTemplateUrl(mNativeTemplateUrlServiceAndroid, keyword);
    }

    /**
     * Finds the search engine type for the given keyword.
     * @param keyword The templateUrl keyword to look up.
     * @return      The search engine type of the specified search engine that contains the keyword.
     */
    public int getSearchEngineTypeFromTemplateUrl(String keyword) {
        return nativeGetSearchEngineTypeFromTemplateUrl(mNativeTemplateUrlServiceAndroid, keyword);
    }

    /**
     * Extracts the search query terms from a SRP URL.
     * @param url The SRP URL.
     * @return A string containing just the search terms stripped from the SRP URL.
     */
    public String extractSearchTermsFromUrl(String url) {
        return nativeExtractSearchTermsFromUrl(mNativeTemplateUrlServiceAndroid, url);
    }

    @VisibleForTesting
    public String addSearchEngineForTesting(String keyword, int ageInDays) {
        return nativeAddSearchEngineForTesting(
                mNativeTemplateUrlServiceAndroid, keyword, ageInDays);
    }

    @VisibleForTesting
    public String updateLastVisitedForTesting(String keyword) {
        return nativeUpdateLastVisitedForTesting(mNativeTemplateUrlServiceAndroid, keyword);
    }

    private native long nativeInit();
    private native void nativeLoad(long nativeTemplateUrlServiceAndroid);
    private native boolean nativeIsLoaded(long nativeTemplateUrlServiceAndroid);
    private native void nativeSetUserSelectedDefaultSearchProvider(
            long nativeTemplateUrlServiceAndroid, String selectedKeyword);
    private native boolean nativeIsDefaultSearchManaged(long nativeTemplateUrlServiceAndroid);
    private native boolean nativeIsSearchResultsPageFromDefaultSearchProvider(
            long nativeTemplateUrlServiceAndroid, String url);
    private native boolean nativeIsSearchByImageAvailable(long nativeTemplateUrlServiceAndroid);
    private native boolean nativeIsDefaultSearchEngineGoogle(long nativeTemplateUrlServiceAndroid);
    private native boolean nativeDoesDefaultSearchEngineHaveLogo(
            long nativeTemplateUrlServiceAndroid);
    private native String nativeGetUrlForSearchQuery(long nativeTemplateUrlServiceAndroid,
            String query);
    private native String nativeGetUrlForVoiceSearchQuery(long nativeTemplateUrlServiceAndroid,
            String query);
    private native String nativeReplaceSearchTermsInUrl(long nativeTemplateUrlServiceAndroid,
            String query, String currentUrl);
    private native String nativeGetUrlForContextualSearchQuery(long nativeTemplateUrlServiceAndroid,
            String query, String alternateTerm, boolean shouldPrefetch, String protocolVersion);
    private native String nativeGetSearchEngineUrlFromTemplateUrl(
            long nativeTemplateUrlServiceAndroid, String keyword);
    private native int nativeGetSearchEngineTypeFromTemplateUrl(
            long nativeTemplateUrlServiceAndroid, String keyword);
    private native String nativeAddSearchEngineForTesting(
            long nativeTemplateUrlServiceAndroid, String keyword, int offset);
    private native String nativeUpdateLastVisitedForTesting(
            long nativeTemplateUrlServiceAndroid, String keyword);
    private native String nativeExtractSearchTermsFromUrl(
            long nativeTemplateUrlServiceAndroid, String url);
    private native void nativeGetTemplateUrls(
            long nativeTemplateUrlServiceAndroid, List<TemplateUrl> templateUrls);
    private native TemplateUrl nativeGetDefaultSearchEngine(long nativeTemplateUrlServiceAndroid);
}
