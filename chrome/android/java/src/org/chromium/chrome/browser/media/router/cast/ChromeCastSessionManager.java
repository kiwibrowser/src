// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.cast;

import android.annotation.SuppressLint;

import com.google.android.gms.cast.ApplicationMetadata;
import com.google.android.gms.cast.Cast;
import com.google.android.gms.cast.CastStatusCodes;

import org.chromium.base.Log;

/**
 * Manages the lifetime of the active CastSessions and broadcasts state changes to observers.
 */
public class ChromeCastSessionManager {
    private static final String TAG = "cr_CastSessionMgr";

    // Singleton instance.
    private static ChromeCastSessionManager sInstance;

    /**
     * Defines the events relevant to CastSession state changes.
     * NOTE: This interface is modeled after Cast SDK V3's SessionManagerListener, but does not map
     * exactly to it.
     */
    public interface CastSessionManagerListener {
        // Called right before a session launch is started.
        public void onSessionStarting(
                ChromeCastSessionManager.CastSessionLaunchRequest originalRequest);

        // Called after a successful session launch.
        public void onSessionStarted(CastSession session);

        // Called after a failed session launch.
        public void onSessionStartFailed();

        // Called when a session is shutting down (as a result of user action, or when a new
        // session is being launched).
        public void onSessionStopAction();

        // Called after a session has shut down.
        public void onSessionEnded();
    }

    /**
     * Encapsulates a request to start a cast session. Mostly used to simplify testing.
     */
    public interface CastSessionLaunchRequest {
        // Starts the request.
        // Success or failure will be reported to the ChromeCastSessionManager via the
        // onSessionStarted() and onSessionStartFailed() calls.
        public void start(Cast.Listener listener);

        // Gets the session observer that should be notified of the session launch.
        public CastSessionManagerListener getSessionListener();
    }

    private class CastListener extends Cast.Listener {
        private CastSession mSession;

        CastListener() {}

        void setSession(CastSession session) {
            mSession = session;
        }

        @Override
        public void onApplicationStatusChanged() {
            if (mSession == null) return;

            mSession.updateSessionStatus();
        }

        @Override
        public void onApplicationMetadataChanged(ApplicationMetadata metadata) {
            if (mSession == null) return;

            mSession.updateSessionStatus();
        }

        // TODO(https://crbug.com/635567): Fix this properly.
        @Override
        @SuppressLint("DefaultLocale")
        public void onApplicationDisconnected(int errorCode) {
            if (errorCode != CastStatusCodes.SUCCESS) {
                Log.e(TAG, String.format("Application disconnected with: %d", errorCode));
            }

            // This callback can be called more than once if the application is stopped from Chrome.
            if (mSession == null) return;

            mSession.stopApplication();
            mSession = null;
        }

        @Override
        public void onVolumeChanged() {
            if (mSession == null) return;

            mSession.onVolumeChanged();
        }
    }

    // The current active session. There can only be one active session on Android.
    // NOTE: This is a Chromium abstraction, different from the Cast SDK CastSession.
    private CastSession mSession;

    // Listener tied to |mSession|.
    private CastListener mListener;

    // Request to be started once |mSession| is stopped.
    private CastSessionLaunchRequest mPendingSessionLaunchRequest;

    // Object that initiated the current cast session and that should be notified
    // of changes to the session.
    private CastSessionManagerListener mCurrentSessionListener;

    // Whether we are currently in the process of launching a session.
    private boolean mSessionLaunching = false;

    public static ChromeCastSessionManager get() {
        if (sInstance == null) sInstance = new ChromeCastSessionManager();

        return sInstance;
    }

    /**
     * Should only be used for testing purposes.
     */
    public static void resetInstanceForTesting() {
        sInstance = null;
    }

    private ChromeCastSessionManager() {}

    /**
     * Called after a session successfully launched and was created.
     * @param session the newly created session.
     */
    public void onSessionStarted(CastSession session) {
        assert mSession == null;

        mSession = session;
        mSessionLaunching = false;
        mCurrentSessionListener.onSessionStarted(session);
    }

    /**
     * Called to initiate a new session launch. Will stop the current session if it exists.
     * Calls back into onSessionStarting().
     * Calling this method when we already have a session will stop that session first.
     * Requests will be dropped if we are in the process of launching a new session.
     * @param request the encapsulated information required to launch the session.
     */
    public void requestSessionLaunch(CastSessionLaunchRequest request) {
        // Do not attempt to launch more than one session at a time.
        if (mSessionLaunching) return;

        // Since we only can only have one session, close it before starting a new one.
        if (mSession != null) {
            mPendingSessionLaunchRequest = request;
            mSession.stopApplication();
            return;
        }

        launchSession(request);
    }

    /**
     * Stops the current session.
     */
    public void stopApplication() {
        mSession.stopApplication();
    }

    /**
     * Starts the session request.
     * Will result in a call to onSessionStarted() or onSessionStartFailed() based on the
     * success/failure of the launch.
     */
    private void launchSession(CastSessionLaunchRequest request) {
        assert mSession == null;

        mSessionLaunching = true;

        mCurrentSessionListener = request.getSessionListener();
        mCurrentSessionListener.onSessionStarting(request);

        mListener = new CastListener();

        request.start(mListener);
    }

    /**
     * Called when the session has received a stop action.
     */
    public void onSessionStopAction() {
        mCurrentSessionListener.onSessionStopAction();
    }

    /**
     * Called when the session has ended. The session may no longer be valid at this point.
     */
    public void onSessionEnded() {
        mCurrentSessionListener.onSessionEnded();

        mSession = null;
        mCurrentSessionListener = null;
        mListener = null;

        if (mPendingSessionLaunchRequest != null) {
            launchSession(mPendingSessionLaunchRequest);
            mPendingSessionLaunchRequest = null;
        }
    }

    /**
     * Called when the session has failed to launch.
     */
    public void onSessionStartFailed() {
        mCurrentSessionListener.onSessionStartFailed();
        mCurrentSessionListener = null;
        mListener = null;

        mSessionLaunching = false;
    }
}
