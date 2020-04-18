// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

/**
 * Used to register listeners that can be notified of changes to the position of a view.
 */
public interface PositionObserver {
    public interface Listener {
        /**
         * Called during predraw if the position of the underlying view has changed.
         */
        void onPositionChanged(int positionX, int positionY);
    }

    /**
     * @return The current x position of the observed view.
     */
    int getPositionX();

    /**
     * @return The current y position of the observed view.
     */
    int getPositionY();

    /**
     * Register a listener to be called when the position of the underlying view changes.
     */
    void addListener(Listener listener);

    /**
     * Remove a previously installed listener.
     */
    void removeListener(Listener listener);

    /**
     * Clears registerned listener(s).
     */
    void clearListener();
}
