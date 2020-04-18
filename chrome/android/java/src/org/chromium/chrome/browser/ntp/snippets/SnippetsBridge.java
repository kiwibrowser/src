// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.snippets;

import android.graphics.Bitmap;

import org.chromium.base.Callback;
import org.chromium.base.ObserverList;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.ntp.cards.SuggestionsCategoryInfo;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.ContentSuggestionsAdditionalAction;

import java.util.ArrayList;
import java.util.List;

/**
 * Provides access to the snippets to display on the NTP using the C++ ContentSuggestionsService.
 */
public class SnippetsBridge implements SuggestionsSource {
    private long mNativeSnippetsBridge;
    private final ObserverList<Observer> mObserverList = new ObserverList<>();

    public static boolean isCategoryStatusAvailable(@CategoryStatus int status) {
        // Note: This code is duplicated in category_status.cc.
        return status == CategoryStatus.AVAILABLE_LOADING || status == CategoryStatus.AVAILABLE;
    }

    public static boolean isCategoryRemote(@CategoryInt int category) {
        return category > KnownCategories.REMOTE_CATEGORIES_OFFSET;
    }

    /** Returns whether the category is considered "enabled", and can show content suggestions. */
    public static boolean isCategoryEnabled(@CategoryStatus int status) {
        switch (status) {
            case CategoryStatus.INITIALIZING:
            case CategoryStatus.AVAILABLE:
            case CategoryStatus.AVAILABLE_LOADING:
                return true;
        }
        return false;
    }

    public static boolean isCategoryLoading(@CategoryStatus int status) {
        return status == CategoryStatus.AVAILABLE_LOADING || status == CategoryStatus.INITIALIZING;
    }

    /**
     * Creates a SnippetsBridge for getting snippet data for the current user.
     *
     * @param profile Profile of the user that we will retrieve snippets for.
     */
    public SnippetsBridge(Profile profile) {
        mNativeSnippetsBridge = nativeInit(profile);
    }

    @Override
    public void destroy() {
        assert mNativeSnippetsBridge != 0;
        nativeDestroy(mNativeSnippetsBridge);
        mNativeSnippetsBridge = 0;
        mObserverList.clear();
    }

    /**
     * Notifies that Chrome on Android has been upgraded.
     */
    public static void onBrowserUpgraded() {
        nativeRemoteSuggestionsSchedulerOnBrowserUpgraded();
    }

    /**
     * Notifies that the persistent fetching scheduler woke up.
     */
    public static void onPersistentSchedulerWakeUp() {
        nativeRemoteSuggestionsSchedulerOnPersistentSchedulerWakeUp();
    }

    @Override
    public boolean areRemoteSuggestionsEnabled() {
        assert mNativeSnippetsBridge != 0;
        return nativeAreRemoteSuggestionsEnabled(mNativeSnippetsBridge);
    }

    public static void setContentSuggestionsNotificationsEnabled(boolean enabled) {
        nativeSetContentSuggestionsNotificationsEnabled(enabled);
    }

    public static boolean areContentSuggestionsNotificationsEnabled() {
        return nativeAreContentSuggestionsNotificationsEnabled();
    }

    @Override
    public void fetchRemoteSuggestions() {
        assert mNativeSnippetsBridge != 0;
        nativeReloadSuggestions(mNativeSnippetsBridge);
    }

    @Override
    public int[] getCategories() {
        assert mNativeSnippetsBridge != 0;
        return nativeGetCategories(mNativeSnippetsBridge);
    }

    @Override
    @CategoryStatus
    public int getCategoryStatus(int category) {
        assert mNativeSnippetsBridge != 0;
        return nativeGetCategoryStatus(mNativeSnippetsBridge, category);
    }

    @Override
    public SuggestionsCategoryInfo getCategoryInfo(int category) {
        assert mNativeSnippetsBridge != 0;
        return nativeGetCategoryInfo(mNativeSnippetsBridge, category);
    }

    @Override
    public List<SnippetArticle> getSuggestionsForCategory(int category) {
        assert mNativeSnippetsBridge != 0;
        return nativeGetSuggestionsForCategory(mNativeSnippetsBridge, category);
    }

    @Override
    public void fetchSuggestionImage(SnippetArticle suggestion, Callback<Bitmap> callback) {
        assert mNativeSnippetsBridge != 0;
        nativeFetchSuggestionImage(mNativeSnippetsBridge, suggestion.mCategory,
                suggestion.mIdWithinCategory, callback);
    }

    @Override
    public void fetchSuggestionFavicon(SnippetArticle suggestion, int minimumSizePx,
            int desiredSizePx, Callback<Bitmap> callback) {
        assert mNativeSnippetsBridge != 0;
        nativeFetchSuggestionFavicon(mNativeSnippetsBridge, suggestion.mCategory,
                suggestion.mIdWithinCategory, minimumSizePx, desiredSizePx, callback);
    }

    @Override
    public void fetchContextualSuggestions(String url, Callback<List<SnippetArticle>> callback) {
    }

    @Override
    public void fetchContextualSuggestionImage(
            SnippetArticle suggestion, Callback<Bitmap> callback) {
    }

    @Override
    public void dismissSuggestion(SnippetArticle suggestion) {
        assert mNativeSnippetsBridge != 0;
        nativeDismissSuggestion(mNativeSnippetsBridge, suggestion.mUrl, suggestion.getGlobalRank(),
                suggestion.mCategory, suggestion.getPerSectionRank(), suggestion.mIdWithinCategory);
    }

    @Override
    public void dismissCategory(@CategoryInt int category) {
        assert mNativeSnippetsBridge != 0;
        nativeDismissCategory(mNativeSnippetsBridge, category);
    }

    @Override
    public void restoreDismissedCategories() {
        assert mNativeSnippetsBridge != 0;
        nativeRestoreDismissedCategories(mNativeSnippetsBridge);
    }

    @Override
    public void addObserver(Observer observer) {
        assert observer != null;
        mObserverList.addObserver(observer);
    }

    @Override
    public void removeObserver(Observer observer) {
        mObserverList.removeObserver(observer);
    }

    @Override
    public void fetchSuggestions(@CategoryInt int category, String[] displayedSuggestionIds,
            Callback<List<SnippetArticle>> successCallback, Runnable failureRunnable) {
        assert mNativeSnippetsBridge != 0;
        // We have nice JNI support for Callbacks but not for Runnables, so wrap the Runnable
        // in a Callback and discard the parameter.
        // TODO(peconn): Use a Runnable here if they get nice JNI support.
        nativeFetch(mNativeSnippetsBridge, category, displayedSuggestionIds, successCallback,
                ignored -> failureRunnable.run());
    }

    @CalledByNative
    private static List<SnippetArticle> createSuggestionList() {
        return new ArrayList<>();
    }

    @CalledByNative
    private static SnippetArticle addSuggestion(List<SnippetArticle> suggestions, int category,
            String id, String title, String publisher, String url, long timestamp, float score,
            long fetchTime, boolean isVideoSuggestion, int thumbnailDominantColor) {
        int position = suggestions.size();
        // thumbnailDominantColor equal to 0 encodes absence of the value. 0 is not a valid color,
        // because the passed color cannot be fully transparent.
        suggestions.add(new SnippetArticle(category, id, title, publisher, url, timestamp, score,
                fetchTime, isVideoSuggestion,
                thumbnailDominantColor == 0 ? null : thumbnailDominantColor));
        return suggestions.get(position);
    }

    @CalledByNative
    private static void setAssetDownloadDataForSuggestion(
            SnippetArticle suggestion, String downloadGuid, String filePath, String mimeType) {
        suggestion.setAssetDownloadData(downloadGuid, filePath, mimeType);
    }

    @CalledByNative
    private static void setOfflinePageDownloadDataForSuggestion(
            SnippetArticle suggestion, long offlinePageId) {
        suggestion.setOfflinePageDownloadData(offlinePageId);
    }

    @CalledByNative
    private static SuggestionsCategoryInfo createSuggestionsCategoryInfo(int category, String title,
            @ContentSuggestionsCardLayout int cardLayout,
            @ContentSuggestionsAdditionalAction int additionalAction, boolean showIfEmpty,
            String noSuggestionsMessage) {
        return new SuggestionsCategoryInfo(
                category, title, cardLayout, additionalAction, showIfEmpty, noSuggestionsMessage);
    }

    @CalledByNative
    private void onNewSuggestions(@CategoryInt int category) {
        for (Observer observer : mObserverList) observer.onNewSuggestions(category);
    }

    @CalledByNative
    private void onCategoryStatusChanged(@CategoryInt int category, @CategoryStatus int newStatus) {
        for (Observer observer : mObserverList) {
            observer.onCategoryStatusChanged(category, newStatus);
        }
    }

    @CalledByNative
    private void onSuggestionInvalidated(@CategoryInt int category, String idWithinCategory) {
        for (Observer observer : mObserverList) {
            observer.onSuggestionInvalidated(category, idWithinCategory);
        }
    }

    @CalledByNative
    private void onFullRefreshRequired() {
        for (Observer observer : mObserverList) observer.onFullRefreshRequired();
    }

    @CalledByNative
    private void onSuggestionsVisibilityChanged(@CategoryInt int category) {
        for (Observer observer : mObserverList) observer.onSuggestionsVisibilityChanged(category);
    }

    private native long nativeInit(Profile profile);
    private native void nativeDestroy(long nativeNTPSnippetsBridge);
    private native void nativeReloadSuggestions(long nativeNTPSnippetsBridge);
    private static native void nativeRemoteSuggestionsSchedulerOnPersistentSchedulerWakeUp();
    private static native void nativeRemoteSuggestionsSchedulerOnBrowserUpgraded();
    private native boolean nativeAreRemoteSuggestionsEnabled(long nativeNTPSnippetsBridge);
    private static native void nativeSetContentSuggestionsNotificationsEnabled(boolean enabled);
    private static native boolean nativeAreContentSuggestionsNotificationsEnabled();
    private native int[] nativeGetCategories(long nativeNTPSnippetsBridge);
    private native int nativeGetCategoryStatus(long nativeNTPSnippetsBridge, int category);
    private native SuggestionsCategoryInfo nativeGetCategoryInfo(
            long nativeNTPSnippetsBridge, int category);
    private native List<SnippetArticle> nativeGetSuggestionsForCategory(
            long nativeNTPSnippetsBridge, int category);
    private native void nativeFetchSuggestionImage(long nativeNTPSnippetsBridge, int category,
            String idWithinCategory, Callback<Bitmap> callback);
    private native void nativeFetchSuggestionFavicon(long nativeNTPSnippetsBridge, int category,
            String idWithinCategory, int minimumSizePx, int desiredSizePx,
            Callback<Bitmap> callback);
    private native void nativeFetch(long nativeNTPSnippetsBridge, int category,
            String[] knownSuggestions, Callback<List<SnippetArticle>> successCallback,
            Callback<Integer> failureCallback);
    private native void nativeDismissSuggestion(long nativeNTPSnippetsBridge, String url,
            int globalPosition, int category, int positionInCategory, String idWithinCategory);
    private native void nativeDismissCategory(long nativeNTPSnippetsBridge, int category);
    private native void nativeRestoreDismissedCategories(long nativeNTPSnippetsBridge);
}
