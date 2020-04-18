// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.PorterDuff.Mode;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.support.annotation.MainThread;
import android.support.annotation.Nullable;
import android.support.v7.content.res.AppCompatResources;

import org.chromium.base.ObserverList;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.profiles.ProfileDownloader;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.ProfileDataSource;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Fetches and caches Google Account profile images and full names for the accounts on the device.
 * ProfileDataCache doesn't observe account list changes by itself, so account list
 * should be provided by calling {@link #update(List)}
 */
@MainThread
public class ProfileDataCache implements ProfileDownloader.Observer, ProfileDataSource.Observer {
    /**
     * Observer to get notifications about changes in profile data.
     */
    public interface Observer {
        /**
         * Notifies that an account's profile data has been updated.
         * @param accountId An account ID.
         */
        void onProfileDataUpdated(String accountId);
    }

    /**
     * Encapsulates info necessary to overlay a circular badge (e.g., child account icon) on top of
     * a user avatar.
     */
    public static class BadgeConfig {
        private final Bitmap mBadge;
        private final Point mPosition;
        private final int mBorderSize;

        /**
         * @param badge Square badge bitmap to overlay on user avatar. Will be cropped to a circular
         *         one while overlaying.
         * @param position Position of top left corner of a badge relative to top left corner of
         *         avatar.
         * @param borderSize Size of a transparent border around badge.
         */
        public BadgeConfig(Bitmap badge, Point position, int borderSize) {
            assert badge.getHeight() == badge.getWidth();
            assert position != null;

            mBadge = badge;
            mPosition = position;
            mBorderSize = borderSize;
        }

        Bitmap getBadge() {
            return mBadge;
        }

        Point getPosition() {
            return mPosition;
        }

        int getBorderSize() {
            return mBorderSize;
        }
    }

    private final Context mContext;
    private final int mImageSize;
    private @Nullable final BadgeConfig mBadgeConfig;
    private final Drawable mPlaceholderImage;
    private final ObserverList<Observer> mObservers = new ObserverList<>();
    private final Map<String, DisplayableProfileData> mCachedProfileData = new HashMap<>();
    private @Nullable final ProfileDataSource mProfileDataSource;

    public ProfileDataCache(Context context, int imageSize) {
        this(context, imageSize, null);
    }

    public ProfileDataCache(Context context, int imageSize, @Nullable BadgeConfig badgeConfig) {
        mContext = context;
        mImageSize = imageSize;
        mBadgeConfig = badgeConfig;

        mPlaceholderImage =
                AppCompatResources.getDrawable(context, R.drawable.logo_avatar_anonymous);

        mProfileDataSource = AccountManagerFacade.get().getProfileDataSource();
    }

    /**
     * Initiate fetching the user accounts data (images and the full name). Fetched data will be
     * sent to observers of ProfileDownloader. The instance must have at least one observer (see
     * {@link #addObserver}) when this method is called.
     */
    public void update(List<String> accounts) {
        ThreadUtils.assertOnUiThread();
        assert !mObservers.isEmpty();

        // ProfileDataSource is updated automatically.
        if (mProfileDataSource != null) return;

        for (int i = 0; i < accounts.size(); i++) {
            if (mCachedProfileData.get(accounts.get(i)) == null) {
                ProfileDownloader.startFetchingAccountInfoFor(
                        mContext, accounts.get(i), mImageSize, true);
            }
        }
    }

    /**
     * @return The {@link DisplayableProfileData} containing the profile data corresponding to the
     *         given account or a {@link DisplayableProfileData} with a placeholder image and null
     *         full and given name.
     */
    public DisplayableProfileData getProfileDataOrDefault(String accountName) {
        DisplayableProfileData profileData = mCachedProfileData.get(accountName);
        if (profileData == null) {
            return new DisplayableProfileData(accountName, mPlaceholderImage, null, null);
        }
        return profileData;
    }

    /**
     * @param observer Observer that should be notified when new profile images are available.
     */
    public void addObserver(Observer observer) {
        ThreadUtils.assertOnUiThread();
        if (mObservers.isEmpty()) {
            if (mProfileDataSource != null) {
                mProfileDataSource.addObserver(this);
                populateCacheFromProfileDataSource();
            } else {
                ProfileDownloader.addObserver(this);
            }
        }
        mObservers.addObserver(observer);
    }

    /**
     * @param observer Observer that was added by {@link #addObserver} and should be removed.
     */
    public void removeObserver(Observer observer) {
        ThreadUtils.assertOnUiThread();
        mObservers.removeObserver(observer);
        if (mObservers.isEmpty()) {
            if (mProfileDataSource != null) {
                mProfileDataSource.removeObserver(this);
            } else {
                ProfileDownloader.removeObserver(this);
            }
        }
    }

    private void populateCacheFromProfileDataSource() {
        for (Map.Entry<String, ProfileDataSource.ProfileData> entry :
                mProfileDataSource.getProfileDataMap().entrySet()) {
            mCachedProfileData.put(entry.getKey(), createDisplayableProfileData(entry.getValue()));
        }
    }

    private DisplayableProfileData createDisplayableProfileData(
            ProfileDataSource.ProfileData profileData) {
        return new DisplayableProfileData(profileData.getAccountName(),
                prepareAvatar(profileData.getAvatar()), profileData.getFullName(),
                profileData.getGivenName());
    }

    @Override
    public void onProfileDownloaded(String accountId, String fullName, String givenName,
            Bitmap bitmap) {
        ThreadUtils.assertOnUiThread();
        mCachedProfileData.put(accountId,
                new DisplayableProfileData(accountId, prepareAvatar(bitmap), fullName, givenName));
        for (Observer observer : mObservers) {
            observer.onProfileDataUpdated(accountId);
        }
    }

    @Override
    public void onProfileDataUpdated(String accountId) {
        assert mProfileDataSource != null;
        ProfileDataSource.ProfileData profileData =
                mProfileDataSource.getProfileDataForAccount(accountId);
        if (profileData == null) {
            mCachedProfileData.remove(accountId);
        } else {
            mCachedProfileData.put(accountId, createDisplayableProfileData(profileData));
        }

        for (Observer observer : mObservers) {
            observer.onProfileDataUpdated(accountId);
        }
    }

    /**
     * Crops avatar image into a circle.
     */
    public static Drawable makeRoundAvatar(Resources resources, Bitmap bitmap) {
        if (bitmap == null) return null;

        Bitmap output = Bitmap.createBitmap(
                bitmap.getWidth(), bitmap.getHeight(), Config.ARGB_8888);
        Canvas canvas = new Canvas(output);

        final Paint paint = new Paint();
        final Rect rect = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());

        paint.setAntiAlias(true);
        canvas.drawARGB(0, 0, 0, 0);
        paint.setColor(Color.WHITE);

        canvas.drawCircle(
                bitmap.getWidth() / 2f, bitmap.getHeight() / 2f, bitmap.getWidth() / 2f, paint);
        paint.setXfermode(new PorterDuffXfermode(Mode.SRC_IN));
        canvas.drawBitmap(bitmap, rect, rect, paint);

        return new BitmapDrawable(resources, output);
    }

    private Drawable prepareAvatar(Bitmap bitmap) {
        Drawable croppedAvatar = bitmap != null ? makeRoundAvatar(mContext.getResources(), bitmap)
                                                : mPlaceholderImage;
        if (mBadgeConfig == null) {
            return croppedAvatar;
        }
        return overlayBadgeOnUserPicture(croppedAvatar);
    }

    private Drawable overlayBadgeOnUserPicture(Drawable userPicture) {
        int badgeSize = mBadgeConfig.getBadge().getHeight();
        int badgedPictureWidth = Math.max(mBadgeConfig.getPosition().x + badgeSize, mImageSize);
        int badgedPictureHeight = Math.max(mBadgeConfig.getPosition().y + badgeSize, mImageSize);
        Bitmap badgedPicture = Bitmap.createBitmap(
                badgedPictureWidth, badgedPictureHeight, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(badgedPicture);
        userPicture.setBounds(0, 0, mImageSize, mImageSize);
        userPicture.draw(canvas);

        // Cut a transparent hole through the background image.
        // This will serve as a border to the badge being overlaid.
        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
        int badgeRadius = badgeSize / 2;
        int badgeCenterX = mBadgeConfig.getPosition().x + badgeRadius;
        int badgeCenterY = mBadgeConfig.getPosition().y + badgeRadius;
        canvas.drawCircle(
                badgeCenterX, badgeCenterY, badgeRadius + mBadgeConfig.getBorderSize(), paint);

        // Draw the badge
        canvas.drawBitmap(mBadgeConfig.getBadge(), mBadgeConfig.getPosition().x,
                mBadgeConfig.getPosition().y, null);
        return new BitmapDrawable(mContext.getResources(), badgedPicture);
    }
}
