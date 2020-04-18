// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import android.content.Context;
import android.os.Handler;
import android.os.SystemClock;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.view.View;

import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.fullscreen.ChromeFullscreenManager;
import org.chromium.chrome.browser.fullscreen.ChromeFullscreenManager.FullscreenListener;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.chrome.browser.widget.ListMenuButton;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.StateChangeReason;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheetObserver;
import org.chromium.chrome.browser.widget.bottomsheet.EmptyBottomSheetObserver;
import org.chromium.chrome.browser.widget.textbubble.ImageTextBubble;
import org.chromium.chrome.browser.widget.textbubble.TextBubble;
import org.chromium.components.feature_engagement.EventConstants;
import org.chromium.components.feature_engagement.FeatureConstants;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.content_public.browser.GestureListenerManager;
import org.chromium.content_public.browser.GestureStateListener;
import org.chromium.content_public.browser.RenderCoordinates;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.widget.ViewRectProvider;

import java.util.Collections;
import java.util.List;

/**
 * A mediator for the contextual suggestions UI component responsible for interacting with
 * the contextual suggestions C++ components (via a bridge), updating the model, and communicating
 * with the component coordinator(s).
 */
class ContextualSuggestionsMediator
        implements EnabledStateMonitor.Observer, FetchHelper.Delegate, ListMenuButton.Delegate {
    private static final float INVALID_PERCENTAGE = -1f;
    private static boolean sOverrideBrowserControlsHiddenForTesting;

    private final Profile mProfile;
    private final TabModelSelector mTabModelSelector;
    private final ContextualSuggestionsCoordinator mCoordinator;
    private final ContextualSuggestionsModel mModel;
    private final ChromeFullscreenManager mFullscreenManager;
    private final View mIphParentView;
    private final EnabledStateMonitor mEnabledStateMonitor;
    private final GestureStateListener mGestureStateListener;
    private final Handler mHandler = new Handler();
    private final int mToolbarTransitionDuration;

    private @Nullable ContextualSuggestionsSource mSuggestionsSource;
    private @Nullable FetchHelper mFetchHelper;
    private @Nullable String mCurrentRequestUrl;
    private @Nullable BottomSheetObserver mSheetObserver;
    private @Nullable TextBubble mHelpBubble;
    private @Nullable WebContents mCurrentWebContents;

    private boolean mDidSuggestionsShowForTab;
    private boolean mHasRecordedPeekEventForTab;

    private boolean mHasReachedTargetScrollPercentage;
    private boolean mHasPeekDelayPassed;
    private boolean mUpdateRemainingCountOnNextPeek;
    private float mRemainingPeekCount;
    private float mTargetScrollPercentage = INVALID_PERCENTAGE;

    /** Whether the content sheet is observed to be opened for the first time. */
    private boolean mHasSheetBeenOpened;

    /**
     * Construct a new {@link ContextualSuggestionsMediator}.
     * @param profile The regular {@link Profile}.
     * @param tabModelSelector The {@link TabModelSelector} for the containing activity.
     * @param fullscreenManager The {@link ChromeFullscreenManager} to listen for browser controls
     *                          events.
     * @param coordinator The {@link ContextualSuggestionsCoordinator} for the component.
     * @param model The {@link ContextualSuggestionsModel} for the component.
     * @param iphParentView The parent {@link View} used to anchor an in-product help bubble.
     */
    ContextualSuggestionsMediator(Profile profile, TabModelSelector tabModelSelector,
            ChromeFullscreenManager fullscreenManager, ContextualSuggestionsCoordinator coordinator,
            ContextualSuggestionsModel model, View iphParentView) {
        mProfile = profile;
        mTabModelSelector = tabModelSelector;
        mCoordinator = coordinator;
        mModel = model;
        mFullscreenManager = fullscreenManager;
        mIphParentView = iphParentView;

        if (ChromeFeatureList.isEnabled(ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_SLIM_PEEK_UI)) {
            mModel.setSlimPeekEnabled(true);
        }
        mToolbarTransitionDuration =
                iphParentView.getContext().getResources().getDimensionPixelSize(
                        R.dimen.contextual_suggestions_toolbar_animation_duration);

        // Create a state monitor that will alert this mediator if the enabled state for contextual
        // suggestions changes.
        mEnabledStateMonitor =
                ContextualSuggestionsDependencyFactory.getInstance().createEnabledStateMonitor(
                        this);

        mGestureStateListener = new GestureStateListener() {
            @Override
            public void onScrollOffsetOrExtentChanged(int scrollOffsetY, int scrollExtentY) {
                if (mHasReachedTargetScrollPercentage) return;

                final RenderCoordinates coordinates =
                        RenderCoordinates.fromWebContents(mCurrentWebContents);
                // Use rounded percentage to avoid the approximated percentage not reaching 100%.
                float percentage = Math.round((float) coordinates.getScrollYPixInt()
                        / coordinates.getMaxVerticalScrollPixInt() * 100f) / 100f;

                if (Float.compare(mTargetScrollPercentage, INVALID_PERCENTAGE) != 0
                        && Float.compare(percentage, mTargetScrollPercentage) >= 0) {
                    mHasReachedTargetScrollPercentage = true;
                    maybeShowContentInSheet();
                }
            }
        };

        fullscreenManager.addListener(new FullscreenListener() {
            @Override
            public void onContentOffsetChanged(float offset) {}

            @Override
            public void onControlsOffsetChanged(
                    float topOffset, float bottomOffset, boolean needsAnimate) {
                maybeShowContentInSheet();
            }

            @Override
            public void onToggleOverlayVideoMode(boolean enabled) {}

            @Override
            public void onBottomControlsHeightChanged(int bottomControlsHeight) {}
        });
    }

    /** Destroys the mediator. */
    void destroy() {
        mEnabledStateMonitor.destroy();

        if (mFetchHelper != null) {
            mFetchHelper.destroy();
            mFetchHelper = null;
        }

        if (mSuggestionsSource != null) {
            mSuggestionsSource.destroy();
            mSuggestionsSource = null;
        }

        if (mHelpBubble != null) mHelpBubble.dismiss();

        if (mCurrentWebContents != null) {
            GestureListenerManager.fromWebContents(mCurrentWebContents)
                    .removeListener(mGestureStateListener);
            mCurrentWebContents = null;
        }
    }

    /** Called when accessibility mode changes. */
    void onAccessibilityModeChanged() {
        mEnabledStateMonitor.onAccessibilityModeChanged();
    }

    /**
     * @return Whether the browser controls are currently completely hidden.
     */
    private boolean areBrowserControlsHidden() {
        return sOverrideBrowserControlsHiddenForTesting
                || MathUtils.areFloatsEqual(-mFullscreenManager.getTopControlOffset(),
                        mFullscreenManager.getTopControlsHeight());
    }

    @Override
    public void onEnabledStateChanged(boolean enabled) {
        if (enabled) {
            mSuggestionsSource = ContextualSuggestionsDependencyFactory.getInstance()
                                         .createContextualSuggestionsSource(mProfile);
            mFetchHelper = ContextualSuggestionsDependencyFactory.getInstance().createFetchHelper(
                    this, mTabModelSelector);
            mFetchHelper.initialize();
        } else {
            clearSuggestions();

            if (mFetchHelper != null) {
                mFetchHelper.destroy();
                mFetchHelper = null;
            }

            if (mSuggestionsSource != null) {
                mSuggestionsSource.destroy();
                mSuggestionsSource = null;
            }
        }
    }

    @Override
    public void onSettingsStateChanged(boolean enabled) {}

    @Override
    public void requestSuggestions(String url) {
        // Guard against null tabs when requesting suggestions. https://crbug.com/836672.
        if (mTabModelSelector.getCurrentTab() == null
                || mTabModelSelector.getCurrentTab().getWebContents() == null) {
            assert false;
            return;
        }

        reportEvent(ContextualSuggestionsEvent.FETCH_REQUESTED);
        mCurrentRequestUrl = url;
        mSuggestionsSource.fetchSuggestions(url, (suggestionsResult) -> {
            if (mTabModelSelector.getCurrentTab() == null
                    || mTabModelSelector.getCurrentTab().getWebContents() == null
                    || mSuggestionsSource == null) {
                return;
            }
            assert mFetchHelper != null;

            // Avoiding double fetches causing suggestions for incorrect context.
            if (!TextUtils.equals(url, mCurrentRequestUrl)) return;

            List<ContextualSuggestionsCluster> clusters = suggestionsResult.getClusters();

            PeekConditions peekConditions = suggestionsResult.getPeekConditions();
            mRemainingPeekCount = peekConditions.getMaximumNumberOfPeeks();
            mTargetScrollPercentage = peekConditions.getPageScrollPercentage();
            long remainingDelay =
                    mFetchHelper.getFetchTimeBaselineMillis(mTabModelSelector.getCurrentTab())
                    + Math.round(peekConditions.getMinimumSecondsOnPage() * 1000)
                    - SystemClock.uptimeMillis();

            assert mCurrentWebContents == null
                : "The current web contents should be cleared before suggestions are requested.";
            mCurrentWebContents = mTabModelSelector.getCurrentTab().getWebContents();
            GestureListenerManager.fromWebContents(mCurrentWebContents)
                    .addListener(mGestureStateListener);

            if (remainingDelay <= 0) {
                // Don't postDelayed if the minimum delay has passed so that the suggestions may
                // be shown through the following call to show contents in the bottom sheet.
                mHasPeekDelayPassed = true;
            } else {
                // Once delay expires, the bottom sheet can be peeked if the browser controls are
                // already hidden, or the next time the browser controls are fully hidden and
                // reshown. Note that this triggering on the latter case is handled by
                // FullscreenListener#onControlsOffsetChanged() in this class.
                mHandler.postDelayed(() -> {
                    mHasPeekDelayPassed = true;
                    maybeShowContentInSheet();
                }, remainingDelay);
            }

            if (clusters.size() > 0 && clusters.get(0).getSuggestions().size() > 0) {
                prepareModel(generateClusterList(clusters), suggestionsResult.getPeekText());
                // If the controls are already off-screen, show the suggestions immediately so they
                // are available on reverse scroll.
                maybeShowContentInSheet();
            }
        });
    }

    @Override
    public void clearState() {
        clearSuggestions();
    }

    @Override
    public void reportFetchDelayed(WebContents webContents) {
        if (mTabModelSelector.getCurrentTab() != null
                && mTabModelSelector.getCurrentTab().getWebContents() == webContents) {
            reportEvent(ContextualSuggestionsEvent.FETCH_DELAYED);
        }
    }

    // ListMenuButton.Delegate implementation.
    @Override
    public ListMenuButton.Item[] getItems() {
        final Context context = ContextUtils.getApplicationContext();
        return new ListMenuButton.Item[] {
                new ListMenuButton.Item(context, R.string.menu_preferences, true),
                new ListMenuButton.Item(context, R.string.menu_send_feedback, true)};
    }

    @Override
    public void onItemSelected(ListMenuButton.Item item) {
        if (item.getTextId() == R.string.menu_preferences) {
            mCoordinator.showSettings();
        } else if (item.getTextId() == R.string.menu_send_feedback) {
            mCoordinator.showFeedback();
        } else {
            assert false : "Unhandled item detected.";
        }
    }

    /**
     * Called when suggestions are cleared either due to the user explicitly dismissing
     * suggestions via the close button or due to the FetchHelper signaling state should
     * be cleared.
     */
    private void clearSuggestions() {
        // TODO(twellington): Does this signal need to go back to FetchHelper?

        // Call remove suggestions before clearing model state so that views don't respond to model
        // changes while suggestions are hiding. See https://crbug.com/840579.
        if (mSheetObserver != null) {
            mCoordinator.removeBottomSheetObserver(mSheetObserver);
            mSheetObserver = null;
        }
        mCoordinator.removeSuggestions();

        mDidSuggestionsShowForTab = false;
        mHasRecordedPeekEventForTab = false;
        mHasSheetBeenOpened = false;
        mHandler.removeCallbacksAndMessages(null);
        mHasReachedTargetScrollPercentage = false;
        mHasPeekDelayPassed = false;
        mUpdateRemainingCountOnNextPeek = false;
        mTargetScrollPercentage = INVALID_PERCENTAGE;
        mRemainingPeekCount = 0f;
        mModel.setClusterList(new ClusterList(Collections.emptyList()));
        mModel.setCloseButtonOnClickListener(null);
        mModel.setMenuButtonVisibility(false);
        if (!mModel.isSlimPeekEnabled()) {
            mModel.setMenuButtonAlpha(0f);
        } else {
            mModel.setToolbarTranslationPercent(1.f);
        }
        mModel.setMenuButtonDelegate(null);
        mModel.setDefaultToolbarClickListener(null);
        mModel.setTitle(null);
        mCurrentRequestUrl = "";

        if (mSuggestionsSource != null) mSuggestionsSource.clearState();

        if (mHelpBubble != null) mHelpBubble.dismiss();

        if (mCurrentWebContents != null) {
            GestureListenerManager.fromWebContents(mCurrentWebContents)
                    .removeListener(mGestureStateListener);
            mCurrentWebContents = null;
        }
    }

    private void prepareModel(ClusterList clusters, String title) {
        if (mSuggestionsSource == null) return;

        mModel.setClusterList(clusters);
        mModel.setCloseButtonOnClickListener(view -> {
            TrackerFactory.getTrackerForProfile(mProfile).notifyEvent(
                    EventConstants.CONTEXTUAL_SUGGESTIONS_DISMISSED);
            @ContextualSuggestionsEvent
            int openedEvent =
                    mHasSheetBeenOpened ? ContextualSuggestionsEvent.UI_DISMISSED_AFTER_OPEN
                                        : ContextualSuggestionsEvent.UI_DISMISSED_WITHOUT_OPEN;
            reportEvent(openedEvent);
            clearSuggestions();
            assert mFetchHelper != null && mTabModelSelector.getCurrentTab() != null;
            mFetchHelper.onSuggestionsDismissed(mTabModelSelector.getCurrentTab());
        });
        mModel.setMenuButtonVisibility(false);
        if (!mModel.isSlimPeekEnabled()) {
            mModel.setMenuButtonAlpha(0f);
        } else {
            mModel.setToolbarTranslationPercent(1.f);
        }
        mModel.setMenuButtonDelegate(this);
        mModel.setDefaultToolbarClickListener(view -> mCoordinator.expandBottomSheet());
        mModel.setTitle(title);
    }

    private void maybeShowContentInSheet() {
        // When the controls scroll completely off-screen, the suggestions are "shown" but
        // remain hidden since their offset from the bottom of the screen is determined by
        // the top controls.
        if (mDidSuggestionsShowForTab || !mModel.hasSuggestions() || mSuggestionsSource == null
                || !areBrowserControlsHidden() || !mHasReachedTargetScrollPercentage
                || !mHasPeekDelayPassed || !hasRemainingPeek()) {
            return;
        }

        mDidSuggestionsShowForTab = true;
        mUpdateRemainingCountOnNextPeek = true;

        mSheetObserver = new EmptyBottomSheetObserver() {
            @Override
            public void onSheetFullyPeeked() {
                if (mUpdateRemainingCountOnNextPeek) {
                    mUpdateRemainingCountOnNextPeek = false;
                    --mRemainingPeekCount;
                }

                if (mHasRecordedPeekEventForTab) return;
                assert !mHasSheetBeenOpened;

                mHasRecordedPeekEventForTab = true;
                TrackerFactory.getTrackerForProfile(mProfile).notifyEvent(
                        EventConstants.CONTEXTUAL_SUGGESTIONS_PEEKED);
                reportEvent(ContextualSuggestionsEvent.UI_PEEK_REVERSE_SCROLL);

                maybeShowHelpBubble();
            }

            @Override
            public void onSheetOffsetChanged(float heightFraction, float offsetPx) {
                if (mHelpBubble != null) mHelpBubble.dismiss();

                // When sheet is fully hidden, clear suggestions if the sheet is not allowed to peek
                // anymore or reset mUpdateCountOnNextPeek so mRemainingPeekCount is updated the
                // next time the sheet fully peeks.
                if (Float.compare(0f, heightFraction) == 0) {
                    if (hasRemainingPeek()) {
                        mUpdateRemainingCountOnNextPeek = true;
                    } else {
                        clearSuggestions();
                    }
                }

                if (mModel.isSlimPeekEnabled()) updateSlimPeekTranslation(offsetPx);
            }

            @Override
            public void onSheetOpened(@StateChangeReason int reason) {
                if (!mHasSheetBeenOpened) {
                    mHasSheetBeenOpened = true;
                    TrackerFactory.getTrackerForProfile(mProfile).notifyEvent(
                            EventConstants.CONTEXTUAL_SUGGESTIONS_OPENED);
                    mCoordinator.showSuggestions(mSuggestionsSource);
                    reportEvent(ContextualSuggestionsEvent.UI_OPENED);
                }
                mModel.setMenuButtonVisibility(true);
            }

            @Override
            public void onSheetClosed(int reason) {
                mModel.setMenuButtonVisibility(false);
            }

            @Override
            public void onTransitionPeekToHalf(float transitionFraction) {
                // If the slim peek UI is enabled, the menu button alpha will be animated with
                // the rest of the toolbar contents.
                if (!mModel.isSlimPeekEnabled()) mModel.setMenuButtonAlpha(transitionFraction);
            }
        };

        mCoordinator.addBottomSheetObserver(mSheetObserver);
        mCoordinator.showContentInSheet();
    }

    private boolean hasRemainingPeek() {
        return Float.compare(mRemainingPeekCount, 1f) >= 0;
    }

    private void maybeShowHelpBubble() {
        Tracker tracker = TrackerFactory.getTrackerForProfile(mProfile);
        if (!tracker.shouldTriggerHelpUI(FeatureConstants.CONTEXTUAL_SUGGESTIONS_FEATURE)) {
            return;
        }

        int extraInset = mModel.isSlimPeekEnabled()
                ? mIphParentView.getResources().getDimensionPixelSize(
                          R.dimen.contextual_suggestions_slim_peek_inset)
                : 0;

        ViewRectProvider rectProvider = new ViewRectProvider(mIphParentView);
        rectProvider.setInsetPx(0,
                mIphParentView.getResources().getDimensionPixelSize(R.dimen.toolbar_shadow_height)
                        + extraInset,
                0, 0);
        if (mModel.isSlimPeekEnabled()) {
            mHelpBubble = new ImageTextBubble(mIphParentView.getContext(), mIphParentView,
                    R.string.contextual_suggestions_in_product_help,
                    R.string.contextual_suggestions_in_product_help, true, rectProvider,
                    R.drawable.ic_logo_googleg_24dp);
            mModel.setToolbarArrowTintResourceId(R.color.google_blue_500);
        } else {
            mHelpBubble = new TextBubble(mIphParentView.getContext(), mIphParentView,
                    R.string.contextual_suggestions_in_product_help,
                    R.string.contextual_suggestions_in_product_help, rectProvider);
        }

        mHelpBubble.setDismissOnTouchInteraction(true);
        mHelpBubble.addOnDismissListener(() -> {
            tracker.dismissed(FeatureConstants.CONTEXTUAL_SUGGESTIONS_FEATURE);
            mHelpBubble = null;
            mModel.setToolbarArrowTintResourceId(R.color.dark_mode_tint);
        });

        mHelpBubble.show();
    }

    private void reportEvent(@ContextualSuggestionsEvent int event) {
        if (mTabModelSelector.getCurrentTab() == null
                || mTabModelSelector.getCurrentTab().getWebContents() == null) {
            // This method is not expected to be called if the current tab or webcontents are null.
            // If this assert is hit, please alert someone on the Chrome Explore on Content team.
            // See https://crbug.com/836672.
            assert false;
            return;
        }

        mSuggestionsSource.reportEvent(mTabModelSelector.getCurrentTab().getWebContents(), event);
    }

    private ClusterList generateClusterList(List<ContextualSuggestionsCluster> clusters) {
        for (ContextualSuggestionsCluster cluster : clusters) {
            cluster.buildChildren();
        }

        return new ClusterList(clusters);
    }

    private void updateSlimPeekTranslation(float bottomSheetOffsetPx) {
        // When the sheet is closed, the toolbar translation is 1.0 to indicate the main
        // toolbar content is fully translated. As the bottomSheetOffsetPx increases, the
        // toolbar translation percent decreases. At 0.f the main toolbar content is not
        // translated at all.
        float adjustedOffset = bottomSheetOffsetPx - mCoordinator.getSheetPeekHeight();
        float translationPercent =
                adjustedOffset <= 0 ? 1.f : (1.f - (adjustedOffset / mToolbarTransitionDuration));
        // TODO(twellington): Drop out early after 0.f has been sent once.
        mModel.setToolbarTranslationPercent(Math.max(0.f, translationPercent));
    }

    @VisibleForTesting
    void showContentInSheetForTesting(boolean disableScrollPercentage, boolean disablePeekDelay) {
        if (disableScrollPercentage) mHasReachedTargetScrollPercentage = true;
        if (disablePeekDelay) mHasPeekDelayPassed = true;
        maybeShowContentInSheet();
    }

    @VisibleForTesting
    TextBubble getHelpBubbleForTesting() {
        return mHelpBubble;
    }

    @VisibleForTesting
    void setTargetScrollPercentageForTesting(float percentage) {
        mTargetScrollPercentage = percentage;
    }

    @VisibleForTesting
    static void setOverrideBrowserControlsHiddenForTesting(boolean override) {
        sOverrideBrowserControlsHiddenForTesting = override;
    }
}
