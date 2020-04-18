// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.bottombar;

/**
 * The delegate that is notified when the OverlayPanel content is released.
 */
public interface OverlayPanelContentViewDelegate {
    /**
     * Releases the content associated to the OverlayPanel.
     */
    void releaseOverlayPanelContent();
}
