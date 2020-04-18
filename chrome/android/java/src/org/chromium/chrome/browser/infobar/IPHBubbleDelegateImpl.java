// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.content.Context;
import android.view.View;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.DownloadManagerService;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.infobar.IPHInfoBarSupport.PopupState;
import org.chromium.chrome.browser.infobar.IPHInfoBarSupport.TrackerParameters;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.widget.textbubble.TextBubble;
import org.chromium.components.feature_engagement.EventConstants;
import org.chromium.components.feature_engagement.FeatureConstants;
import org.chromium.components.feature_engagement.Tracker;

/**
 * Default implementation of {@link IPHInfoBarSupport.IPHBubbleDelegate} that handles interacting
 * with {@link Tracker} and creating a {@link TextBubble} based on the type of infobar
 * shown.
 */
class IPHBubbleDelegateImpl implements IPHInfoBarSupport.IPHBubbleDelegate {
    private final Context mContext;
    private final Tracker mTracker;

    IPHBubbleDelegateImpl(Context context) {
        mContext = context;
        Profile profile = Profile.getLastUsedProfile();
        mTracker = TrackerFactory.getTrackerForProfile(profile);
    }

    // IPHInfoBarSupport.IPHBubbleDelegate implementation.
    @Override
    public PopupState createStateForInfoBar(View anchorView, @InfoBarIdentifier int infoBarId) {
        logEvent(infoBarId);
        TrackerParameters params = getTrackerParameters(infoBarId);
        if (params == null || !mTracker.shouldTriggerHelpUI(params.feature)) return null;

        PopupState state = new PopupState();
        state.view = anchorView;
        state.feature = params.feature;
        state.bubble = new TextBubble(
                mContext, anchorView, params.textId, params.accessibilityTextId, anchorView);
        state.bubble.setDismissOnTouchInteraction(true);

        return state;
    }

    @Override
    public void onPopupDismissed(PopupState state) {
        mTracker.dismissed(state.feature);
    }

    private void logEvent(@InfoBarIdentifier int infoBarId) {
        switch (infoBarId) {
            case InfoBarIdentifier.DATA_REDUCTION_PROXY_PREVIEW_INFOBAR_DELEGATE:
                mTracker.notifyEvent(EventConstants.DATA_SAVER_PREVIEW_INFOBAR_SHOWN);
                break;
            default:
                break;
        }
    }

    private TrackerParameters getTrackerParameters(@InfoBarIdentifier int infoBarId) {
        switch (infoBarId) {
            case InfoBarIdentifier.DATA_REDUCTION_PROXY_PREVIEW_INFOBAR_DELEGATE:
                return new TrackerParameters(FeatureConstants.DATA_SAVER_PREVIEW_FEATURE,
                        R.string.iph_data_saver_preview_text, R.string.iph_data_saver_preview_text);
            case InfoBarIdentifier.DOWNLOAD_PROGRESS_INFOBAR_ANDROID:
                return DownloadManagerService.getDownloadManagerService()
                        .getInfoBarController(Profile.getLastUsedProfile().isOffTheRecord())
                        .getTrackerParameters();
            default:
                return null;
        }
    }
}
