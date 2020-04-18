// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.share;

import android.app.Activity;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import org.chromium.chrome.browser.share.ShareHelper.TargetChosenCallback;
import org.chromium.components.dom_distiller.core.DomDistillerUrlUtils;

/**
 * A container object for passing share parameters to {@link ShareHelper}.
 */
public class ShareParams {
    /**
     * Whether it should share directly with the activity that was most recently used to share.
     * If false, the share selection will be saved.
     */
    private final boolean mShareDirectly;

    /** Whether to save the chosen activity for future direct sharing. */
    private final boolean mSaveLastUsed;

    /** The activity that is used to access package manager. */
    private final Activity mActivity;

    /** The title of the page to be shared. */
    private final String mTitle;

    /**
     * The text to be shared. If both |text| and |url| are supplied, they are concatenated with a
     * space.
     */
    private final String mText;

    /** The URL of the page to be shared. */
    private final String mUrl;

    /** The Uri to the offline MHTML file to be shared. */
    private final Uri mOfflineUri;

    /** The Uri of the screenshot of the page to be shared. */
    private final Uri mScreenshotUri;

    /**
     * Optional callback to be called when user makes a choice. Will not be called if receiving a
     * response when the user makes a choice is not supported (on older Android versions).
     */
    private TargetChosenCallback mCallback;

    /** The package name of the app who requests for share. If Null, it is requested by Chrome */
    private final String mSourcePackageName;

    /** The {@link Runnable} called when the share dialog is dismissed. */
    @Nullable
    private final Runnable mOnDialogDismissed;

    private ShareParams(boolean shareDirectly, boolean saveLastUsed, Activity activity,
            String title, String text, String url, @Nullable Uri offlineUri,
            @Nullable Uri screenshotUri, @Nullable TargetChosenCallback callback,
            @Nullable String sourcePackageName, @Nullable Runnable onDialogDismissed) {
        mShareDirectly = shareDirectly;
        mSaveLastUsed = saveLastUsed;
        mActivity = activity;
        mTitle = title;
        mText = text;
        mUrl = url;
        mOfflineUri = offlineUri;
        mScreenshotUri = screenshotUri;
        mCallback = callback;
        mSourcePackageName = sourcePackageName;
        mOnDialogDismissed = onDialogDismissed;
    }

    /**
     * @return Whether it should share directly with the activity that was most recently used to
     * share.
     */
    public boolean shareDirectly() {
        return mShareDirectly;
    }

    /**
     * @return Whether to save the chosen activity for future direct sharing.
     */
    public boolean saveLastUsed() {
        return mSaveLastUsed;
    }

    /**
     * @return The activity that is used to access package manager.
     */
    public Activity getActivity() {
        return mActivity;
    }

    /**
     * @return The title of the page to be shared.
     */
    public String getTitle() {
        return mTitle;
    }

    /**
     * @return The text to be shared.
     */
    public String getText() {
        return mText;
    }

    /**
     * @return The URL of the page to be shared.
     */
    public String getUrl() {
        return mUrl;
    }

    /**
     * @return The Uri to the offline MHTML file to be shared.
     */
    @Nullable
    public Uri getOfflineUri() {
        return mOfflineUri;
    }

    /**
     * @return The Uri of the screenshot of the page to be shared.
     */
    @Nullable
    public Uri getScreenshotUri() {
        return mScreenshotUri;
    }

    /**
     * @return The callback to be called when user makes a choice.
     */
    @Nullable
    public TargetChosenCallback getCallback() {
        return mCallback;
    }

    /**
     * @return The package name of the app who requests for share.
     */
    public String getSourcePackageName() {
        return mSourcePackageName;
    }

    /**
     * @return The {@link Runnable} to be called when the share dialog is dismissed.
     */
    @Nullable
    public Runnable getOnDialogDismissed() {
        return mOnDialogDismissed;
    }

    /** The builder for {@link ShareParams} objects. */
    public static class Builder {
        private boolean mShareDirectly;
        private boolean mSaveLastUsed;
        private Activity mActivity;
        private String mTitle;
        private String mText;
        private String mUrl;
        private Uri mOfflineUri;
        private Uri mScreenshotUri;
        private TargetChosenCallback mCallback;
        private String mSourcePackageName;
        private boolean mIsExternalUrl;
        private Runnable mOnDialogDismissed;

        public Builder(@NonNull Activity activity, @NonNull String title, @NonNull String url) {
            mActivity = activity;
            mUrl = url;
            mTitle = title;
        }

        /**
         * Sets the text to be shared.
         */
        public Builder setText(@NonNull String text) {
            mText = text;
            return this;
        }

        /**
         * Sets whether it should share directly with the activity that was most recently used to
         * share.
         */
        public Builder setShareDirectly(boolean shareDirectly) {
            mShareDirectly = shareDirectly;
            return this;
        }

        /**
         * Sets whether to save the chosen activity for future direct sharing.
         */
        public Builder setSaveLastUsed(boolean saveLastUsed) {
            mSaveLastUsed = saveLastUsed;
            return this;
        }

        /**
         * Sets the URL of the page to be shared.
         */
        public Builder setUrl(@NonNull String url) {
            mUrl = url;
            return this;
        }

        /**
         * Sets the Uri of the offline MHTML file to be shared.
         */
        public Builder setOfflineUri(@Nullable Uri offlineUri) {
            mOfflineUri = offlineUri;
            return this;
        }

        /**
         * Sets the Uri of the screenshot of the page to be shared.
         */
        public Builder setScreenshotUri(@Nullable Uri screenshotUri) {
            mScreenshotUri = screenshotUri;
            return this;
        }

        /**
         * Sets the callback to be called when user makes a choice.
         */
        public Builder setCallback(@Nullable TargetChosenCallback callback) {
            mCallback = callback;
            return this;
        }

        /**
         * Set the package name of the app who requests for share.
         */
        public Builder setSourcePackageName(String sourcePackageName) {
            mSourcePackageName = sourcePackageName;
            return this;
        }

        /**
         * Set whether the params are created by the url from external app.
         */
        public Builder setIsExternalUrl(boolean isExternalUrl) {
            mIsExternalUrl = isExternalUrl;
            return this;
        }

        /**
         * Set a runnable to be called when the share dialog is dismissed.
         */
        public Builder setOnDialogDismissed(Runnable onDialogDismised) {
            mOnDialogDismissed = onDialogDismised;
            return this;
        }

        /** @return A fully constructed {@link ShareParams} object. */
        public ShareParams build() {
            if (!TextUtils.isEmpty(mUrl)) {
                if (!mIsExternalUrl) {
                    mUrl = DomDistillerUrlUtils.getOriginalUrlFromDistillerUrl(mUrl);
                }
                if (!TextUtils.isEmpty(mText)) {
                    // Concatenate text and URL with a space.
                    mText = mText + " " + mUrl;
                } else {
                    mText = mUrl;
                }
            }
            return new ShareParams(mShareDirectly, mSaveLastUsed, mActivity, mTitle, mText, mUrl,
                    mOfflineUri, mScreenshotUri, mCallback, mSourcePackageName, mOnDialogDismissed);
        }
    }
}
