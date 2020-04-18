/*
 * Copyright (c) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ims.internal;



/**
 * Provides the APIs to control the media session, such as passing the surface object,
 * controlling the camera (front/rear selection, zoom, brightness, ...) for a video calling.
 *
 * @hide
 */
public class ImsStreamMediaSession {
    private static final String TAG = "ImsStreamMediaSession";

    /**
     * Listener for events relating to an IMS media session.
     * <p>Many of these events are also received by {@link ImsStreamMediaSession.Listener}.</p>
     */
    public static class Listener {
    }

    private Listener mListener;

    ImsStreamMediaSession(IImsStreamMediaSession mediaSession) {
    }

    ImsStreamMediaSession(IImsStreamMediaSession mediaSession, Listener listener) {
        this(mediaSession);
        setListener(listener);
    }

    /**
     * Sets the listener to listen to the media session events. A {@code ImsStreamMediaSession}
     * can only hold one listener at a time. Subsequent calls to this method
     * override the previous listener.
     *
     * @param listener to listen to the media session events of this object
     */
    public void setListener(Listener listener) {
        mListener = listener;
    }
}
