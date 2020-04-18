// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.net.Uri;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.components.url_formatter.UrlFormatter;

/**
 * This InfoBar is shown to let the user know about a blocked Framebust and offer to
 * continue the redirection by tapping on a link.
 */
public class FramebustBlockInfoBar extends InfoBar {
    /** For Log statements. */
    private static final String TAG = "Framebust Infobar";

    private final String mBlockedUrl;

    /** Whether the infobar should be shown as a mini-infobar or a classic expanded one. */
    private boolean mIsExpanded;

    @VisibleForTesting
    public FramebustBlockInfoBar(String blockedUrl) {
        super(R.drawable.infobar_chrome, null, null);
        mBlockedUrl = blockedUrl;
    }

    @Override
    public void onButtonClicked(boolean isPrimaryButton) {
        assert isPrimaryButton;
        onButtonClicked(ActionType.OK);
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        layout.setMessage(getString(R.string.redirect_blocked_message));
        InfoBarControlLayout control = layout.addControlLayout();

        // TODO(crbug.com/834959): remove after bug fixed.
        Log.i(TAG, "Mark possible occurance of crbug.com/834959");

        ViewGroup ellipsizerView =
                (ViewGroup) LayoutInflater.from(getContext())
                        .inflate(R.layout.infobar_control_url_ellipsizer, control, false);

        // Formatting the URL and requesting to omit the scheme might still include it for some of
        // them (e.g. file, filesystem). We split the output of the formatting to make sure we don't
        // end up duplicating it.
        String formattedUrl = UrlFormatter.formatUrlForSecurityDisplay(mBlockedUrl, true);
        String scheme = Uri.parse(mBlockedUrl).getScheme() + "://";

        TextView schemeView = ellipsizerView.findViewById(R.id.url_scheme);
        schemeView.setText(scheme);

        TextView urlView = ellipsizerView.findViewById(R.id.url_minus_scheme);
        urlView.setText(formattedUrl.substring(scheme.length()));

        ellipsizerView.setOnClickListener(view -> onLinkClicked());

        control.addView(ellipsizerView);
        layout.setButtons(
                getContext().getResources().getString(R.string.always_allow_redirects), null);
    }

    @Override
    protected void createCompactLayoutContent(InfoBarCompactLayout layout) {
        new InfoBarCompactLayout.MessageBuilder(layout)
                .withText(getString(R.string.redirect_blocked_short_message))
                .withLink(R.string.details_link, view -> onLinkClicked())
                .buildAndInsert();
    }

    @Override
    protected boolean usesCompactLayout() {
        return !mIsExpanded;
    }

    @Override
    public void onLinkClicked() {
        if (!mIsExpanded) {
            mIsExpanded = true;
            replaceView(createView());
            return;
        }

        super.onLinkClicked();
    }

    @VisibleForTesting
    public String getBlockedUrl() {
        return mBlockedUrl;
    }

    private String getString(@StringRes int stringResId) {
        return getContext().getString(stringResId);
    }

    @CalledByNative
    private static FramebustBlockInfoBar create(String blockedUrl) {
        return new FramebustBlockInfoBar(blockedUrl);
    }
}
