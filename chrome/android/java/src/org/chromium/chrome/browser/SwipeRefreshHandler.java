// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.Context;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;

import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.gesturenav.SideSlideLayout;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabWebContentsUserData;
import org.chromium.components.feature_engagement.EventConstants;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.content_public.browser.WebContents;
import org.chromium.third_party.android.swiperefresh.SwipeRefreshLayout;
import org.chromium.base.ContextUtils;
import org.chromium.ui.OverscrollRefreshHandler;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * An overscroll handler implemented in terms a modified version of the Android
 * compat library's SwipeRefreshLayout effect.
 */
public class SwipeRefreshHandler
        extends TabWebContentsUserData implements OverscrollRefreshHandler {
    /**
     * The targets that can handle MotionEvents.
     */
    @IntDef({SwipeType.NONE, SwipeType.PULL_TO_REFRESH, SwipeType.HISTORY_NAV})
    @Retention(RetentionPolicy.SOURCE)
    private @interface SwipeType {
        int NONE = 0;
        int PULL_TO_REFRESH = 1;
        int HISTORY_NAV = 2;
    }

    private @SwipeType int mSwipeType;

    private static final Class<SwipeRefreshHandler> USER_DATA_KEY = SwipeRefreshHandler.class;

    // Synthetic delay between the {@link #didStopRefreshing()} signal and the
    // call to stop the refresh animation.
    private static final int STOP_REFRESH_ANIMATION_DELAY_MS = 500;

    // Max allowed duration of the refresh animation after a refresh signal,
    // guarding against cases where the page reload fails or takes too long.
    private static final int MAX_REFRESH_ANIMATION_DURATION_MS = 7500;

    private final boolean mNavigationEnabled;

    // The modified AppCompat version of the refresh effect, handling all core
    // logic, rendering and animation.
    private SwipeRefreshLayout mSwipeRefreshLayout;

    // The Tab where the swipe occurs.
    private Tab mTab;

    // The container view the SwipeRefreshHandler instance is currently
    // associated with.
    private ViewGroup mContainerView;

    // Async runnable for ending the refresh animation after the page first
    // loads a frame. This is used to provide a reasonable minimum animation time.
    private Runnable mStopRefreshingRunnable;

    // Handles removing the layout from the view hierarchy.  This is posted to ensure it does not
    // conflict with pending Android draws.
    private Runnable mDetachRefreshLayoutRunnable;

    // Accessibility utterance used to indicate refresh activation.
    private String mAccessibilityRefreshString;

    // History navigation layout and the main logic turning the gesture into corresponding UI.
    private SideSlideLayout mSideSlideLayout;

    // Async runnable for ending the refresh animation after the page first
    // loads a frame. This is used to provide a reasonable minimum animation time.
    private Runnable mStopNavigatingRunnable;

    // Handles removing the layout from the view hierarchy.  This is posted to ensure it does not
    // conflict with pending Android draws.
    private Runnable mDetachSideSlideLayoutRunnable;

    public static SwipeRefreshHandler from(Tab tab) {
        SwipeRefreshHandler handler = get(tab);
        if (handler == null) {
            handler =
                    tab.getUserDataHost().setUserData(USER_DATA_KEY, new SwipeRefreshHandler(tab));
        }
        return handler;
    }

    @Nullable
    public static SwipeRefreshHandler get(Tab tab) {
        return tab.getUserDataHost().getUserData(USER_DATA_KEY);
    }

    /**
     * Simple constructor to use when creating an OverscrollRefresh instance from code.
     *
     * @param tab The Tab where the swipe occurs.
     */
    private SwipeRefreshHandler(Tab tab) {
        super(tab);
        mTab = tab;
        mNavigationEnabled = ContextUtils.getAppSharedPreferences().getBoolean("side_swipe_mode_enabled", true);
    }

    private void initSwipeRefreshLayout() {
        final Context context = mTab.getThemedApplicationContext();
        mSwipeRefreshLayout = new SwipeRefreshLayout(context);
        mSwipeRefreshLayout.setLayoutParams(
                new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
        mSwipeRefreshLayout.setColorSchemeResources(R.color.light_active_color);
        if (mContainerView != null) mSwipeRefreshLayout.setEnabled(true);

        mSwipeRefreshLayout.setOnRefreshListener(() -> {
            cancelStopRefreshingRunnable();
            ThreadUtils.postOnUiThreadDelayed(
                    getStopRefreshingRunnable(), MAX_REFRESH_ANIMATION_DURATION_MS);
            if (mAccessibilityRefreshString == null) {
                int resId = R.string.accessibility_swipe_refresh;
                mAccessibilityRefreshString = context.getResources().getString(resId);
            }
            mSwipeRefreshLayout.announceForAccessibility(mAccessibilityRefreshString);
            mTab.reload();
            RecordUserAction.record("MobilePullGestureReload");
        });
        mSwipeRefreshLayout.setOnResetListener(() -> {
            if (mDetachRefreshLayoutRunnable != null) return;
            mDetachRefreshLayoutRunnable = () -> {
                mDetachRefreshLayoutRunnable = null;
                detachSwipeRefreshLayoutIfNecessary();
            };
            ThreadUtils.postOnUiThread(mDetachRefreshLayoutRunnable);
        });
    }

    private void initSideSlideLayout() {
        mSideSlideLayout = new SideSlideLayout(mTab.getThemedApplicationContext());
        mSideSlideLayout.setLayoutParams(
                new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
        if (mContainerView != null) mSideSlideLayout.setEnabled(true);

        mSideSlideLayout.setOnNavigationListener((isForward) -> {
            if (isForward) {
                mTab.goForward();
            } else {
                mTab.goBack();
            }
            cancelStopNavigatingRunnable();
            mSideSlideLayout.post(getStopNavigatingRunnable());
        });

        mSideSlideLayout.setOnResetListener(() -> {
            if (mDetachSideSlideLayoutRunnable != null) return;
            mDetachSideSlideLayoutRunnable = () -> {
                mDetachSideSlideLayoutRunnable = null;
                detachSideSlideLayoutIfNecessary();
            };
            mSideSlideLayout.post(mDetachSideSlideLayoutRunnable);
        });
    }

    @Override
    public void initWebContents(WebContents webContents) {
        webContents.setOverscrollRefreshHandler(this);
        mContainerView = mTab.getContentView();
        setEnabled(true);
    }

    @Override
    public void cleanupWebContents(WebContents webContents) {
        if (mSwipeRefreshLayout != null) detachSwipeRefreshLayoutIfNecessary();
        if (mSideSlideLayout != null) detachSideSlideLayoutIfNecessary();
        mContainerView = null;
        setEnabled(false);
    }

    @Override
    public void destroyInternal() {
        if (mSwipeRefreshLayout != null) {
            mSwipeRefreshLayout.setOnRefreshListener(null);
            mSwipeRefreshLayout.setOnResetListener(null);
        }
        if (mSideSlideLayout != null) {
            mSideSlideLayout.setOnNavigationListener(null);
            mSideSlideLayout.setOnResetListener(null);
        }
    }

    /**
     * Notify the SwipeRefreshLayout that a refresh action has completed.
     * Defer the notification by a reasonable minimum to ensure sufficient
     * visiblity of the animation.
     */
    public void didStopRefreshing() {
        if (mSwipeRefreshLayout == null || !mSwipeRefreshLayout.isRefreshing()) return;
        cancelStopRefreshingRunnable();
        mSwipeRefreshLayout.postDelayed(
                getStopRefreshingRunnable(), STOP_REFRESH_ANIMATION_DELAY_MS);
    }

    @Override
    public boolean start(float xDelta, float yDelta) {
        if (mTab.getActivity() != null && mTab.getActivity().getBottomSheet() != null) {
            Tracker tracker = TrackerFactory.getTrackerForProfile(Profile.getLastUsedProfile());
            tracker.notifyEvent(EventConstants.PULL_TO_REFRESH);
        }

        if (yDelta > 0 && yDelta > Math.abs(xDelta)) {
            if (mSwipeRefreshLayout == null) initSwipeRefreshLayout();
            mSwipeType = SwipeType.PULL_TO_REFRESH;
            attachSwipeRefreshLayoutIfNecessary();
            return mSwipeRefreshLayout.start();
        } else if (Math.abs(xDelta) > Math.abs(yDelta) && mNavigationEnabled) {
            if (mSideSlideLayout == null) initSideSlideLayout();
            mSwipeType = SwipeType.HISTORY_NAV;
            boolean isForward = xDelta <= 0;
            boolean shouldStart = isForward ? mTab.canGoForward() : mTab.canGoBack();
            if (shouldStart) {
                mSideSlideLayout.setDirection(isForward);
                attachSideSlideLayoutIfNecessary();
                mSideSlideLayout.start();
            }
            return shouldStart;
        }
        mSwipeType = SwipeType.NONE;
        return false;
    }

    @Override
    public void pull(float xDelta, float yDelta) {
        TraceEvent.begin("SwipeRefreshHandler.pull");
        if (mSwipeType == SwipeType.PULL_TO_REFRESH) {
            mSwipeRefreshLayout.pull(yDelta);
        } else if (mSwipeType == SwipeType.HISTORY_NAV) {
            mSideSlideLayout.pull(xDelta);
        }
        TraceEvent.end("SwipeRefreshHandler.pull");
    }

    @Override
    public void release(boolean allowRefresh) {
        TraceEvent.begin("SwipeRefreshHandler.release");
        if (mSwipeType == SwipeType.PULL_TO_REFRESH) {
            mSwipeRefreshLayout.release(allowRefresh);
        } else if (mSwipeType == SwipeType.HISTORY_NAV) {
            mSideSlideLayout.release(allowRefresh);
        }
        TraceEvent.end("SwipeRefreshHandler.release");
    }

    @Override
    public void reset() {
        cancelStopRefreshingRunnable();
        if (mSwipeRefreshLayout != null) mSwipeRefreshLayout.reset();
        cancelStopNavigatingRunnable();
        if (mSideSlideLayout != null) mSideSlideLayout.reset();
    }

    @Override
    public void setEnabled(boolean enabled) {
        if (!enabled) reset();
    }

    private void cancelStopRefreshingRunnable() {
        if (mStopRefreshingRunnable != null) {
            ThreadUtils.getUiThreadHandler().removeCallbacks(mStopRefreshingRunnable);
        }
    }

    private void cancelDetachLayoutRunnable() {
        if (mDetachRefreshLayoutRunnable != null) {
            ThreadUtils.getUiThreadHandler().removeCallbacks(mDetachRefreshLayoutRunnable);
            mDetachRefreshLayoutRunnable = null;
        }
    }

    private Runnable getStopRefreshingRunnable() {
        if (mStopRefreshingRunnable == null) {
            mStopRefreshingRunnable = () -> mSwipeRefreshLayout.setRefreshing(false);
        }
        return mStopRefreshingRunnable;
    }

    // The animation view is attached/detached on-demand to minimize overlap
    // with composited SurfaceView content.
    private void attachSwipeRefreshLayoutIfNecessary() {
        cancelDetachLayoutRunnable();
        if (mSwipeRefreshLayout.getParent() == null) {
            mContainerView.addView(mSwipeRefreshLayout);
        }
    }

    private void detachSwipeRefreshLayoutIfNecessary() {
        cancelDetachLayoutRunnable();
        if (mSwipeRefreshLayout.getParent() != null) {
            mContainerView.removeView(mSwipeRefreshLayout);
        }
    }

    private void cancelStopNavigatingRunnable() {
        if (mStopNavigatingRunnable != null) {
            mSideSlideLayout.removeCallbacks(mStopNavigatingRunnable);
            mStopNavigatingRunnable = null;
        }
    }

    private void cancelDetachSideSlideLayoutRunnable() {
        if (mDetachSideSlideLayoutRunnable != null) {
            mSideSlideLayout.removeCallbacks(mDetachSideSlideLayoutRunnable);
            mDetachSideSlideLayoutRunnable = null;
        }
    }

    private Runnable getStopNavigatingRunnable() {
        if (mStopNavigatingRunnable == null) {
            mStopNavigatingRunnable = () -> mSideSlideLayout.stopNavigating();
        }
        return mStopNavigatingRunnable;
    }

    private void attachSideSlideLayoutIfNecessary() {
        cancelDetachSideSlideLayoutRunnable();
        if (mSideSlideLayout.getParent() == null) {
            mContainerView.addView(mSideSlideLayout);
        }
    }

    private void detachSideSlideLayoutIfNecessary() {
        cancelDetachSideSlideLayoutRunnable();
        if (mSideSlideLayout.getParent() != null) {
            mContainerView.removeView(mSideSlideLayout);
        }
    }
}
