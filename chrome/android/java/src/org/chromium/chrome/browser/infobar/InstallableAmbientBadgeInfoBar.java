// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import static android.view.View.IMPORTANT_FOR_ACCESSIBILITY_NO;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ResourceId;
import org.chromium.chrome.browser.widget.accessibility.AccessibleTextView;

/**
 * An ambient infobar to tell the user that the current site they are visiting is a PWA.
 */
public class InstallableAmbientBadgeInfoBar extends InfoBar implements View.OnClickListener {
    private String mMessageText;
    private String mUrl;
    private boolean mIsHiding;

    @CalledByNative
    private static InfoBar show(
            int enumeratedIconId, Bitmap iconBitmap, String messageText, String url) {
        int drawableId = ResourceId.mapToDrawableId(enumeratedIconId);
        return new InstallableAmbientBadgeInfoBar(drawableId, iconBitmap, messageText, url);
    }

    @Override
    protected boolean usesCompactLayout() {
        return true;
    }

    @Override
    protected void onStartedHiding() {
        mIsHiding = true;
    }

    @Override
    public void createCompactLayoutContent(InfoBarCompactLayout layout) {
        TextView prompt = new AccessibleTextView(getContext());

        Resources res = layout.getResources();
        prompt.setText(mMessageText);
        prompt.setTextSize(TypedValue.COMPLEX_UNIT_PX, res.getDimension(R.dimen.infobar_text_size));
        prompt.setTextColor(ApiCompatibilityUtils.getColor(res, R.color.google_blue_700));
        prompt.setGravity(Gravity.CENTER_VERTICAL);
        prompt.setOnClickListener(this);

        ImageView iconView = layout.findViewById(R.id.infobar_icon);
        int iconMargin = res.getDimensionPixelSize(R.dimen.infobar_small_icon_margin);
        iconView.setPadding(iconMargin, 0, iconMargin, 0);

        iconView.setOnClickListener(this);
        iconView.setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);
        final int messagePadding =
                res.getDimensionPixelOffset(R.dimen.reader_mode_infobar_text_padding);
        prompt.setPadding(0, messagePadding, 0, messagePadding);
        layout.addContent(prompt, 1f);
    }

    /**
     * Triggers opening the app or add to home screen when the infobar's text or icon is clicked.
     */
    @Override
    public void onClick(View v) {
        if (getNativeInfoBarPtr() == 0 || mIsHiding) return;

        nativeAddToHomescreen(getNativeInfoBarPtr());
    }

    /**
     * Creates the infobar.
     * @param iconDrawableId    Drawable ID corresponding to the icon that the infobar will show.
     * @param iconBitmap        Bitmap of the icon to display in the infobar.
     * @param messageText       String to display
     */
    private InstallableAmbientBadgeInfoBar(
            int iconDrawableId, Bitmap iconBitmap, String messageText, String url) {
        super(iconDrawableId, iconBitmap, null);
        mMessageText = messageText;
        mUrl = url;
    }

    private native void nativeAddToHomescreen(long nativeInstallableAmbientBadgeInfoBar);
}
