// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tab;

import android.support.annotation.IntDef;
import android.view.View;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.base.ObserverList.RewindableIterator;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.media.MediaCaptureNotificationService;
import org.chromium.chrome.browser.policy.PolicyAuditor;
import org.chromium.chrome.browser.policy.PolicyAuditor.AuditEvent;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;

/**
 * WebContentsObserver used by Tab.
 */
public class TabWebContentsObserver extends WebContentsObserver {
    // URL didFailLoad error code. Should match the value in net_error_list.h.
    public static final int BLOCKED_BY_ADMINISTRATOR = -22;

    /** Used for logging. */
    private static final String TAG = "TabWebContentsObs";

    // TabRendererCrashStatus defined in tools/metrics/histograms/histograms.xml.
    private static final int TAB_RENDERER_CRASH_STATUS_SHOWN_IN_FOREGROUND_APP = 0;
    private static final int TAB_RENDERER_CRASH_STATUS_HIDDEN_IN_FOREGROUND_APP = 1;
    private static final int TAB_RENDERER_CRASH_STATUS_HIDDEN_IN_BACKGROUND_APP = 2;
    private static final int TAB_RENDERER_CRASH_STATUS_MAX = 3;

    // TabRendererExitStatus defined in tools/metrics/histograms/histograms.xml.
    // Designed to replace TabRendererCrashStatus if numbers line up.
    @IntDef({TAB_RENDERER_EXIT_STATUS_OOM_PROTECTED_IN_RUNNING_APP,
        TAB_RENDERER_EXIT_STATUS_OOM_PROTECTED_IN_PAUSED_APP,
        TAB_RENDERER_EXIT_STATUS_OOM_PROTECTED_IN_BACKGROUND_APP,
        TAB_RENDERER_EXIT_STATUS_NOT_PROTECTED_IN_RUNNING_APP,
        TAB_RENDERER_EXIT_STATUS_NOT_PROTECTED_IN_PAUSED_APP,
        TAB_RENDERER_EXIT_STATUS_NOT_PROTECTED_IN_BACKGROUND_APP,
        TAB_RENDERER_EXIT_STATUS_MAX})
    private @interface TabRendererExitStatus {}
    private static final int TAB_RENDERER_EXIT_STATUS_OOM_PROTECTED_IN_RUNNING_APP = 0;
    private static final int TAB_RENDERER_EXIT_STATUS_OOM_PROTECTED_IN_PAUSED_APP = 1;
    private static final int TAB_RENDERER_EXIT_STATUS_OOM_PROTECTED_IN_BACKGROUND_APP = 2;
    private static final int TAB_RENDERER_EXIT_STATUS_NOT_PROTECTED_IN_RUNNING_APP = 3;
    private static final int TAB_RENDERER_EXIT_STATUS_NOT_PROTECTED_IN_PAUSED_APP = 4;
    private static final int TAB_RENDERER_EXIT_STATUS_NOT_PROTECTED_IN_BACKGROUND_APP = 5;
    private static final int TAB_RENDERER_EXIT_STATUS_MAX = 6;

    private final Tab mTab;

    public TabWebContentsObserver(WebContents webContents, Tab tab) {
        super(webContents);
        mTab = tab;
    }

    @Override
    public void renderProcessGone(boolean processWasOomProtected) {
        Log.i(TAG, "renderProcessGone() for tab id: " + mTab.getId()
                + ", oom protected: " + Boolean.toString(processWasOomProtected)
                + ", already needs reload: " + Boolean.toString(mTab.needsReload()));
        // Do nothing for subsequent calls that happen while the tab remains crashed. This
        // can occur when the tab is in the background and it shares the renderer with other
        // tabs. After the renderer crashes, the WebContents of its tabs are still around
        // and they still share the RenderProcessHost. When one of the tabs reloads spawning
        // a new renderer for the shared RenderProcessHost and the new renderer crashes
        // again, all tabs sharing this renderer will be notified about the crash (including
        // potential background tabs that did not reload yet).
        if (mTab.needsReload() || mTab.isShowingSadTab()) return;

        // This will replace TabRendererCrashStatus if numbers line up.
        int appState = ApplicationStatus.getStateForApplication();
        boolean applicationRunning = (appState == ApplicationState.HAS_RUNNING_ACTIVITIES);
        boolean applicationPaused = (appState == ApplicationState.HAS_PAUSED_ACTIVITIES);
        @TabRendererExitStatus int rendererExitStatus = TAB_RENDERER_EXIT_STATUS_MAX;
        if (processWasOomProtected) {
            if (applicationRunning) {
                rendererExitStatus = TAB_RENDERER_EXIT_STATUS_OOM_PROTECTED_IN_RUNNING_APP;
            } else if (applicationPaused) {
                rendererExitStatus = TAB_RENDERER_EXIT_STATUS_OOM_PROTECTED_IN_PAUSED_APP;
            } else {
                rendererExitStatus = TAB_RENDERER_EXIT_STATUS_OOM_PROTECTED_IN_BACKGROUND_APP;
            }
        } else {
            if (applicationRunning) {
                rendererExitStatus = TAB_RENDERER_EXIT_STATUS_NOT_PROTECTED_IN_RUNNING_APP;
            } else if (applicationPaused) {
                rendererExitStatus = TAB_RENDERER_EXIT_STATUS_NOT_PROTECTED_IN_PAUSED_APP;
            } else {
                rendererExitStatus = TAB_RENDERER_EXIT_STATUS_NOT_PROTECTED_IN_BACKGROUND_APP;
            }
        }
        RecordHistogram.recordEnumeratedHistogram(
                "Tab.RendererExitStatus", rendererExitStatus, TAB_RENDERER_EXIT_STATUS_MAX);

        int activityState = ApplicationStatus.getStateForActivity(
                mTab.getWindowAndroid().getActivity().get());
        int rendererCrashStatus = TAB_RENDERER_CRASH_STATUS_MAX;
        if (mTab.isHidden() || activityState == ActivityState.PAUSED
                || activityState == ActivityState.STOPPED
                || activityState == ActivityState.DESTROYED) {
            // The tab crashed in background or was killed by the OS out-of-memory killer.
            mTab.setNeedsReload();
            if (applicationRunning) {
                rendererCrashStatus = TAB_RENDERER_CRASH_STATUS_HIDDEN_IN_FOREGROUND_APP;
            } else {
                rendererCrashStatus = TAB_RENDERER_CRASH_STATUS_HIDDEN_IN_BACKGROUND_APP;
            }
        } else {
            rendererCrashStatus = TAB_RENDERER_CRASH_STATUS_SHOWN_IN_FOREGROUND_APP;
            mTab.showSadTab();
            // This is necessary to correlate histogram data with stability counts.
            RecordHistogram.recordBooleanHistogram("Stability.Android.RendererCrash", true);
        }
        RecordHistogram.recordEnumeratedHistogram(
                "Tab.RendererCrashStatus", rendererCrashStatus, TAB_RENDERER_CRASH_STATUS_MAX);

        mTab.handleTabCrash();
    }

    @Override
    public void didFinishLoad(long frameId, String validatedUrl, boolean isMainFrame) {
        if (mTab.getNativePage() != null) {
            mTab.pushNativePageStateToNavigationEntry();
        }
        if (isMainFrame) mTab.didFinishPageLoad();
        PolicyAuditor auditor = AppHooks.get().getPolicyAuditor();
        auditor.notifyAuditEvent(
                mTab.getApplicationContext(), AuditEvent.OPEN_URL_SUCCESS, validatedUrl, "");
    }

    @Override
    public void didFailLoad(
            boolean isMainFrame, int errorCode, String description, String failingUrl) {
        mTab.updateThemeColorIfNeeded(true);
        RewindableIterator<TabObserver> observers = mTab.getTabObservers();
        while (observers.hasNext()) {
            observers.next().onDidFailLoad(mTab, isMainFrame, errorCode, description, failingUrl);
        }

        if (isMainFrame) mTab.didFailPageLoad(errorCode);

        recordErrorInPolicyAuditor(failingUrl, description, errorCode);
    }

    private void recordErrorInPolicyAuditor(String failingUrl, String description, int errorCode) {
        assert description != null;

        PolicyAuditor auditor = AppHooks.get().getPolicyAuditor();
        auditor.notifyAuditEvent(mTab.getApplicationContext(), AuditEvent.OPEN_URL_FAILURE,
                failingUrl, description);
        if (errorCode == BLOCKED_BY_ADMINISTRATOR) {
            auditor.notifyAuditEvent(
                    mTab.getApplicationContext(), AuditEvent.OPEN_URL_BLOCKED, failingUrl, "");
        }
    }

    @Override
    public void titleWasSet(String title) {
        mTab.updateTitle(title);
    }

    @Override
    public void didStartNavigation(
            String url, boolean isInMainFrame, boolean isSameDocument, boolean isErrorPage) {
        if (isInMainFrame && !isSameDocument) {
            mTab.didStartPageLoad(url, isErrorPage);
        }

        RewindableIterator<TabObserver> observers = mTab.getTabObservers();
        while (observers.hasNext()) {
            observers.next().onDidStartNavigation(
                    mTab, url, isInMainFrame, isSameDocument, isErrorPage);
        }
    }

    @Override
    public void didFinishNavigation(String url, boolean isInMainFrame, boolean isErrorPage,
            boolean hasCommitted, boolean isSameDocument, boolean isFragmentNavigation,
            Integer pageTransition, int errorCode, String errorDescription, int httpStatusCode) {
        RewindableIterator<TabObserver> observers = mTab.getTabObservers();
        while (observers.hasNext()) {
            observers.next().onDidFinishNavigation(mTab, url, isInMainFrame, isErrorPage,
                    hasCommitted, isSameDocument, isFragmentNavigation, pageTransition, errorCode,
                    httpStatusCode);
        }

        if (errorCode != 0) {
            mTab.updateThemeColorIfNeeded(true);
            if (isInMainFrame) mTab.didFailPageLoad(errorCode);

            recordErrorInPolicyAuditor(url, errorDescription, errorCode);
        }

        if (!hasCommitted) return;

        if (isInMainFrame) {
            mTab.setIsTabStateDirty(true);
            mTab.updateTitle();
            mTab.handleDidFinishNavigation(url, pageTransition);
            mTab.setIsShowingErrorPage(isErrorPage);

            observers.rewind();
            while (observers.hasNext()) {
                observers.next().onUrlUpdated(mTab);
            }
        }

        FullscreenManager fullscreenManager = mTab.getFullscreenManager();
        if (isInMainFrame && !isSameDocument && fullscreenManager != null) {
            fullscreenManager.exitPersistentFullscreenMode();
        }

        if (isInMainFrame) {
            mTab.stopSwipeRefreshHandler();
        }
    }

    @Override
    public void didFirstVisuallyNonEmptyPaint() {
        RewindableIterator<TabObserver> observers = mTab.getTabObservers();
        while (observers.hasNext()) {
            observers.next().didFirstVisuallyNonEmptyPaint(mTab);
        }
    }

    @Override
    public void didChangeThemeColor(int color) {
        mTab.updateThemeColorIfNeeded(true);
    }

    @Override
    public void didAttachInterstitialPage() {
        mTab.getInfoBarContainer().setVisibility(View.INVISIBLE);
        mTab.showRenderedPage();
        mTab.updateThemeColorIfNeeded(false);

        RewindableIterator<TabObserver> observers = mTab.getTabObservers();
        while (observers.hasNext()) {
            observers.next().onDidAttachInterstitialPage(mTab);
        }
        mTab.notifyLoadProgress(mTab.getProgress());

        mTab.updateFullscreenEnabledState();

        PolicyAuditor auditor = AppHooks.get().getPolicyAuditor();
        auditor.notifyCertificateFailure(
                PolicyAuditor.nativeGetCertificateFailure(mTab.getWebContents()),
                mTab.getApplicationContext());
    }

    @Override
    public void didDetachInterstitialPage() {
        mTab.getInfoBarContainer().setVisibility(View.VISIBLE);
        mTab.updateThemeColorIfNeeded(false);

        RewindableIterator<TabObserver> observers = mTab.getTabObservers();
        while (observers.hasNext()) {
            observers.next().onDidDetachInterstitialPage(mTab);
        }
        mTab.notifyLoadProgress(mTab.getProgress());

        mTab.updateFullscreenEnabledState();

        if (!mTab.maybeShowNativePage(mTab.getUrl(), false)) {
            mTab.showRenderedPage();
        }
    }

    @Override
    public void navigationEntriesDeleted() {
        mTab.notifyNavigationEntriesDeleted();
    }

    @Override
    public void destroy() {
        MediaCaptureNotificationService.updateMediaNotificationForTab(
                mTab.getApplicationContext(), mTab.getId(), 0, mTab.getUrl());
        super.destroy();
    }
}
