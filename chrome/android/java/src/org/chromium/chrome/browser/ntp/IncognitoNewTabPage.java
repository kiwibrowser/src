// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import android.app.Activity;
import android.graphics.Canvas;
import android.support.v4.view.ViewCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BasicNativePage;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.NativePageHost;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.compositor.layouts.content.InvalidationAwareThumbnailProvider;
import org.chromium.chrome.browser.help.HelpAndFeedback;
import org.chromium.chrome.browser.ntp.IncognitoNewTabPageView.IncognitoNewTabPageManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.vr_shell.VrShellDelegate;

/**
 * Provides functionality when the user interacts with the Incognito NTP.
 */
public class IncognitoNewTabPage
        extends BasicNativePage implements InvalidationAwareThumbnailProvider {
    private Activity mActivity;

    private String mTitle;
    protected IncognitoNewTabPageView mIncognitoNewTabPageView;

    private boolean mIsLoaded;

    private IncognitoNewTabPageManager mIncognitoNewTabPageManager;

    private void showIncognitoLearnMore() {
        HelpAndFeedback.getInstance(mActivity).show(mActivity,
                mActivity.getString(R.string.help_context_incognito_learn_more),
                Profile.getLastUsedProfile(), null);
    }

    /**
     * Constructs an Incognito NewTabPage.
     * @param activity The activity used to create the new tab page's View.
     */
    public IncognitoNewTabPage(ChromeActivity activity, NativePageHost host) {
        super(activity, host, true);
    }

    @Override
    protected void initialize(ChromeActivity activity, final NativePageHost host) {
        mActivity = activity;

        mIncognitoNewTabPageManager = new IncognitoNewTabPageManager() {
            @Override
            public void loadIncognitoLearnMore() {
                // Incognito 'Learn More' either opens a new activity or a new non-incognito tab.
                // Both is not supported in VR. Request to exit VR and if we succeed show the 'Learn
                // More' page in 2D.
                if (VrShellDelegate.isInVr()) {
                    VrShellDelegate.requestToExitVrAndRunOnSuccess(
                            IncognitoNewTabPage.this ::showIncognitoLearnMore);
                    return;
                }
                showIncognitoLearnMore();
            }

            @Override
            public void onLoadingComplete() {
                mIsLoaded = true;
            }
        };

        mTitle = activity.getResources().getString(R.string.button_new_tab);

        LayoutInflater inflater = LayoutInflater.from(activity);
        mIncognitoNewTabPageView =
                (IncognitoNewTabPageView) inflater.inflate(getLayoutResource(), null);
        mIncognitoNewTabPageView.initialize(mIncognitoNewTabPageManager);

        if (!useMDIncognitoNTP()) {
            TextView newTabIncognitoMessage = (TextView) mIncognitoNewTabPageView.findViewById(
                    R.id.new_tab_incognito_message);
            newTabIncognitoMessage.setText(
                    activity.getResources().getString(R.string.new_tab_incognito_message));
        }
    }

    protected int getLayoutResource() {
        return useMDIncognitoNTP() ? R.layout.new_tab_page_incognito_md
                                   : R.layout.new_tab_page_incognito;
    }

    protected static boolean useMDIncognitoNTP() {
        return ChromeFeatureList.isEnabled(ChromeFeatureList.MATERIAL_DESIGN_INCOGNITO_NTP);
    }

    /**
     * @return Whether the NTP has finished loaded.
     */
    @VisibleForTesting
    public boolean isLoadedForTests() {
        return mIsLoaded;
    }

    // NativePage overrides

    @Override
    public void destroy() {
        assert !ViewCompat
                .isAttachedToWindow(getView()) : "Destroy called before removed from window";
    }

    @Override
    public String getUrl() {
        return UrlConstants.NTP_URL;
    }

    @Override
    public String getTitle() {
        return mTitle;
    }

    @Override
    public boolean needsToolbarShadow() {
        return true;
    }

    @Override
    public View getView() {
        return mIncognitoNewTabPageView;
    }

    @Override
    public String getHost() {
        return UrlConstants.NTP_HOST;
    }

    @Override
    public void updateForUrl(String url) {
    }

    // InvalidationAwareThumbnailProvider

    @Override
    public boolean shouldCaptureThumbnail() {
        return mIncognitoNewTabPageView.shouldCaptureThumbnail();
    }

    @Override
    public void captureThumbnail(Canvas canvas) {
        mIncognitoNewTabPageView.captureThumbnail(canvas);
    }
}
