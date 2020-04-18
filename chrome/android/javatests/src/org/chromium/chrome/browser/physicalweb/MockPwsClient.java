// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.physicalweb;

import android.graphics.Bitmap;

import org.chromium.chrome.browser.physicalweb.PwsClient.FetchIconCallback;
import org.chromium.chrome.browser.physicalweb.PwsClient.ResolveScanCallback;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * This class sends requests to the Physical Web Service.
 */
class MockPwsClient implements PwsClient {
    private List<Collection<UrlInfo>> mResolveCalls;
    private List<String> mFetchIconCalls;
    private List<List<PwsResult>> mPwsResults;
    private List<Bitmap> mIconBitmaps;

    public MockPwsClient() {
        mResolveCalls = new ArrayList<>();
        mFetchIconCalls = new ArrayList<>();
        mPwsResults = new ArrayList<>();
        mIconBitmaps = new ArrayList<>();
    }

    public List<Collection<UrlInfo>> getResolveCalls() {
        return mResolveCalls;
    }

    public List<String> getFetchIconCalls() {
        return mFetchIconCalls;
    }

    public void addPwsResults(List<PwsResult> pwsResults) {
        mPwsResults.add(pwsResults);
    }

    public void addPwsResult(PwsResult pwsResult) {
        List<PwsResult> pwsResults = new ArrayList<>();
        pwsResults.add(pwsResult);
        addPwsResults(pwsResults);
    }

    public void addCombinedPwsResults() {
        List<PwsResult> pwsResults = new ArrayList<>();
        for (List<PwsResult> subList : mPwsResults) {
            pwsResults.addAll(subList);
        }
        addPwsResults(pwsResults);
    }

    public void addIconBitmap(Bitmap iconBitmap) {
        mIconBitmaps.add(iconBitmap);
    }

    /**
     * Send an HTTP request to the PWS to resolve a set of URLs.
     * @param broadcastUrls The URLs to resolve.
     * @param resolveScanCallback The callback to be run when the response is received.
     */
    @Override
    public void resolve(final Collection<UrlInfo> broadcastUrls,
            final ResolveScanCallback resolveScanCallback) {
        mResolveCalls.add(broadcastUrls);
        resolveScanCallback.onPwsResults(mPwsResults.remove(0));
    }

    /**
     * Send an HTTP request to fetch a favicon.
     * @param iconUrl The URL of the favicon.
     * @param fetchIconCallback The callback to be run when the icon is received.
     */
    @Override
    public void fetchIcon(final String iconUrl, final FetchIconCallback fetchIconCallback) {
        mFetchIconCalls.add(iconUrl);
        fetchIconCallback.onIconReceived(iconUrl, mIconBitmaps.remove(0));
    }
}
