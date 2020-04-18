// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

/**
 * An infobar to notify that the generated password was saved.
 */
public class GeneratedPasswordSavedInfoBar extends ConfirmInfoBar {
    private final int mInlineLinkRangeStart;
    private final int mInlineLinkRangeEnd;

    /**
     * Creates and shows the infobar to notify that the generated password was saved.
     * @param iconDrawableId Drawable ID corresponding to the icon that the infobar will show.
     * @param messageText Message to display in the infobar.
     * @param inlineLinkRangeStart The start of the range of the messageText that should be a link.
     * @param inlineLinkRangeEnd The end of the range of the messageText that should be a link.
     * @param buttonLabel String to display on the button.
     */
    public GeneratedPasswordSavedInfoBar(int iconDrawableId, String messageText,
            int inlineLinkRangeStart, int inlineLinkRangeEnd, String buttonLabel) {
        super(iconDrawableId, null, messageText, null, buttonLabel, null);
        mInlineLinkRangeStart = inlineLinkRangeStart;
        mInlineLinkRangeEnd = inlineLinkRangeEnd;
    }

    /**
     * Used to specify button layout and custom content. Makes infobar display a single button and
     * an inline link in the message.
     * @param layout Handles user interface for the infobar.
     */
    @Override
    public void createContent(InfoBarLayout layout) {
        super.createContent(layout);
        layout.setInlineMessageLink(mInlineLinkRangeStart, mInlineLinkRangeEnd);
    }
}
