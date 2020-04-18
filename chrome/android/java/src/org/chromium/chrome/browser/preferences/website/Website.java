// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.website;

import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.browser.ContentSettingsType;
import org.chromium.chrome.browser.preferences.website.WebsitePreferenceBridge.StorageInfoClearedCallback;
import org.chromium.chrome.browser.util.MathUtils;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

/**
 * Website is a class for storing information about a website and its associated permissions.
 */
public class Website implements Serializable {

    static final int INVALID_CAMERA_OR_MICROPHONE_ACCESS = 0;
    static final int CAMERA_ACCESS_ALLOWED = 1;
    static final int MICROPHONE_AND_CAMERA_ACCESS_ALLOWED = 2;
    static final int MICROPHONE_ACCESS_ALLOWED = 3;
    static final int CAMERA_ACCESS_DENIED = 4;
    static final int MICROPHONE_AND_CAMERA_ACCESS_DENIED = 5;
    static final int MICROPHONE_ACCESS_DENIED = 6;

    private final WebsiteAddress mOrigin;
    private final WebsiteAddress mEmbedder;

    private ContentSettingException mAdsException;
    private ContentSettingException mAutoplayExceptionInfo;
    private ContentSettingException mBackgroundSyncExceptionInfo;
    private CameraInfo mCameraInfo;
    private ClipboardInfo mClipboardInfo;
    private ContentSettingException mCookieException;
    private GeolocationInfo mGeolocationInfo;
    private ContentSettingException mJavaScriptException;
    private LocalStorageInfo mLocalStorageInfo;
    private MicrophoneInfo mMicrophoneInfo;
    private MidiInfo mMidiInfo;
    private NotificationInfo mNotificationInfo;
    private ContentSettingException mPopupException;
    private ProtectedMediaIdentifierInfo mProtectedMediaIdentifierInfo;
    private ContentSettingException mSoundException;
    private final List<StorageInfo> mStorageInfo = new ArrayList<StorageInfo>();
    private int mStorageInfoCallbacksLeft;

    // The collection of chooser-based permissions (e.g. USB device access) granted to this site.
    // Each entry declares its own ContentSettingsType and so depending on how this object was
    // built this list could contain multiple types of objects.
    private final List<ChosenObjectInfo> mObjectInfo = new ArrayList<ChosenObjectInfo>();

    public Website(WebsiteAddress origin, WebsiteAddress embedder) {
        mOrigin = origin;
        mEmbedder = embedder;
    }

    public WebsiteAddress getAddress() {
        return mOrigin;
    }

    public String getTitle() {
        return mOrigin.getTitle();
    }

    public String getSummary() {
        if (mEmbedder == null) return null;
        return mEmbedder.getTitle();
    }

    /**
     * A comparison function for sorting by address (first by origin and then
     * by embedder).
     */
    public int compareByAddressTo(Website to) {
        if (this == to) return 0;
        int originComparison = mOrigin.compareTo(to.mOrigin);
        if (originComparison == 0) {
            if (mEmbedder == null) return to.mEmbedder == null ? 0 : -1;
            if (to.mEmbedder == null) return 1;
            return mEmbedder.compareTo(to.mEmbedder);
        }
        return originComparison;
    }

    /**
     * A comparison function for sorting by storage (most used first).
     * @return which site uses more storage.
     */
    public int compareByStorageTo(Website to) {
        if (this == to) return 0;
        return MathUtils.compareLongs(to.getTotalUsage(), getTotalUsage());
    }

    /**
     * Sets the Ads exception info for this Website.
     */
    public void setAdsException(ContentSettingException exception) {
        mAdsException = exception;
    }

    /**
     * Returns the Ads exception info for this Website.
     */
    public ContentSettingException getAdsException() {
        return mAdsException;
    }

    /**
     * Returns what permission governs the Ads setting.
     */
    public ContentSetting getAdsPermission() {
        if (mAdsException != null) {
            return mAdsException.getContentSetting();
        }
        return null;
    }

    /**
     * Sets the Ads permission.
     */
    public void setAdsPermission(ContentSetting value) {
        // It is possible to set the permission without having an existing exception, because we can
        // show the BLOCK state even when this permission is set to the default. In that case, just
        // set an exception now to BLOCK to enable changing the permission.
        if (mAdsException == null) {
            setAdsException(
                    new ContentSettingException(ContentSettingsType.CONTENT_SETTINGS_TYPE_ADS,
                            getAddress().getOrigin(), ContentSetting.BLOCK, ""));
        }
        mAdsException.setContentSetting(value);
    }

    /**
     * Returns what permission governs Autoplay access.
     */
    public ContentSetting getAutoplayPermission() {
        return mAutoplayExceptionInfo != null ? mAutoplayExceptionInfo.getContentSetting() : null;
    }

    /**
     * Configure Autoplay permission access setting for this site.
     */
    public void setAutoplayPermission(ContentSetting value) {
        if (mAutoplayExceptionInfo != null) {
            mAutoplayExceptionInfo.setContentSetting(value);
        }
    }

    /**
     * Returns the Autoplay exception info for this Website.
     */
    public ContentSettingException getAutoplayException() {
        return mAutoplayExceptionInfo;
    }

    /**
     * Sets the Autoplay exception info for this Website.
     */
    public void setAutoplayException(ContentSettingException exception) {
        mAutoplayExceptionInfo = exception;
    }

    /**
     * Returns the background sync exception info for this Website.
     */
    public ContentSettingException getBackgroundSyncException() {
        return mBackgroundSyncExceptionInfo;
    }

    /**
     * Sets the background sync setting exception info for this website.
     */
    public void setBackgroundSyncException(ContentSettingException exception) {
        mBackgroundSyncExceptionInfo = exception;
    }

    /**
     * @return what permission governs background sync.
     */
    public ContentSetting getBackgroundSyncPermission() {
        return mBackgroundSyncExceptionInfo != null
                ? mBackgroundSyncExceptionInfo.getContentSetting()
                : null;
    }

    /**
     * Configures the background sync setting for this site.
     */
    public void setBackgroundSyncPermission(ContentSetting value) {
        if (mBackgroundSyncExceptionInfo != null) {
            mBackgroundSyncExceptionInfo.setContentSetting(value);
        }
    }

    /**
     * Sets camera capture info class.
     */
    public void setCameraInfo(CameraInfo info) {
        mCameraInfo = info;
    }

    public CameraInfo getCameraInfo() {
        return mCameraInfo;
    }

    /**
     * Returns what setting governs camera capture access.
     */
    public ContentSetting getCameraPermission() {
        return mCameraInfo != null ? mCameraInfo.getContentSetting() : null;
    }

    /**
     * Configure camera capture setting for this site.
     */
    public void setCameraPermission(ContentSetting value) {
        if (mCameraInfo != null) mCameraInfo.setContentSetting(value);
    }

    /**
     * Sets the ClipboardInfo object for this Website.
     */
    public void setClipboardInfo(ClipboardInfo info) {
        mClipboardInfo = info;
    }

    public ClipboardInfo getClipboardInfo() {
        return mClipboardInfo;
    }

    /**
     * Returns what permission governs Clipboard access.
     */
    public ContentSetting getClipboardPermission() {
        return mClipboardInfo != null ? mClipboardInfo.getContentSetting() : null;
    }

    /**
     * Configure Clipboard permission access setting for this site.
     */
    public void setClipboardPermission(ContentSetting value) {
        if (mClipboardInfo != null) mClipboardInfo.setContentSetting(value);
    }

    /**
     * Sets the Cookie exception info for this site.
     */
    public void setCookieException(ContentSettingException exception) {
        mCookieException = exception;
    }

    public ContentSettingException getCookieException() {
        return mCookieException;
    }

    /**
     * Gets the permission that governs cookie preferences.
     */
    public ContentSetting getCookiePermission() {
        return mCookieException != null ? mCookieException.getContentSetting() : null;
    }

    /**
     * Sets the permission that govers cookie preferences for this site.
     */
    public void setCookiePermission(ContentSetting value) {
        if (mCookieException != null) {
            mCookieException.setContentSetting(value);
        }
    }

    /**
     * Sets the GeoLocationInfo object for this Website.
     */
    public void setGeolocationInfo(GeolocationInfo info) {
        mGeolocationInfo = info;
    }

    public GeolocationInfo getGeolocationInfo() {
        return mGeolocationInfo;
    }

    /**
     * Returns what permission governs geolocation access.
     */
    public ContentSetting getGeolocationPermission() {
        return mGeolocationInfo != null ? mGeolocationInfo.getContentSetting() : null;
    }

    /**
     * Configure geolocation access setting for this site.
     */
    public void setGeolocationPermission(ContentSetting value) {
        if (mGeolocationInfo != null) {
            mGeolocationInfo.setContentSetting(value);
        }
    }

    /**
     * Returns what permission governs JavaScript access.
     */
    public ContentSetting getJavaScriptPermission() {
        return mJavaScriptException != null ? mJavaScriptException.getContentSetting() : null;
    }

    /**
     * Configure JavaScript permission access setting for this site.
     */
    public void setJavaScriptPermission(ContentSetting value) {
        if (mJavaScriptException != null) {
            mJavaScriptException.setContentSetting(value);
        }
    }

    /**
     * Sets the JavaScript exception info for this Website.
     */
    public void setJavaScriptException(ContentSettingException exception) {
        mJavaScriptException = exception;
    }

    /**
     * Returns the JavaScript exception info for this Website.
     */
    public ContentSettingException getJavaScriptException() {
        return mJavaScriptException;
    }

    /**
     * Returns what permission governs Sound access.
     */
    public ContentSetting getSoundPermission() {
        return mSoundException != null ? mSoundException.getContentSetting() : null;
    }

    /**
     * Configure Sound permission access setting for this site.
     */
    public void setSoundPermission(ContentSetting value) {
        // It is possible to set the permission without having an existing exception, because we
        // always show the sound permission in Site Settings.
        if (mSoundException == null) {
            setSoundException(
                    new ContentSettingException(ContentSettingsType.CONTENT_SETTINGS_TYPE_SOUND,
                            getAddress().getHost(), value, ""));
        }
        // We want this to be called even after calling setSoundException above because this will
        // trigger the actual change on the PrefServiceBridge.
        mSoundException.setContentSetting(value);
        if (value == ContentSetting.BLOCK) {
            RecordUserAction.record("SoundContentSetting.MuteBy.SiteSettings");
        } else {
            RecordUserAction.record("SoundContentSetting.UnmuteBy.SiteSettings");
        }
    }

    /**
     * Sets the Sound exception info for this Website.
     */
    public void setSoundException(ContentSettingException exception) {
        mSoundException = exception;
    }

    /**
     * Returns the Sound exception info for this Website.
     */
    public ContentSettingException getSoundException() {
        return mSoundException;
    }

    /**
     * Sets microphone capture info class.
     */
    public void setMicrophoneInfo(MicrophoneInfo info) {
        mMicrophoneInfo = info;
    }

    public MicrophoneInfo getMicrophoneInfo() {
        return mMicrophoneInfo;
    }

    /**
     * Returns what setting governs microphone capture access.
     */
    public ContentSetting getMicrophonePermission() {
        return mMicrophoneInfo != null ? mMicrophoneInfo.getContentSetting() : null;
    }

    /**
     * Configure microphone capture setting for this site.
     */
    public void setMicrophonePermission(ContentSetting value) {
        if (mMicrophoneInfo != null) mMicrophoneInfo.setContentSetting(value);
    }

    /**
     * Sets the MidiInfo object for this Website.
     */
    public void setMidiInfo(MidiInfo info) {
        mMidiInfo = info;
    }

    public MidiInfo getMidiInfo() {
        return mMidiInfo;
    }

    /**
     * Returns what permission governs MIDI usage access.
     */
    public ContentSetting getMidiPermission() {
        return mMidiInfo != null ? mMidiInfo.getContentSetting() : null;
    }

    /**
     * Configure Midi usage access setting for this site.
     */
    public void setMidiPermission(ContentSetting value) {
        if (mMidiInfo != null) {
            mMidiInfo.setContentSetting(value);
        }
    }

    /**
     * Sets Notification access permission information class.
     */
    public void setNotificationInfo(NotificationInfo info) {
        mNotificationInfo = info;
    }

    public NotificationInfo getNotificationInfo() {
        return mNotificationInfo;
    }

    /**
     * Returns what setting governs notification access.
     */
    public ContentSetting getNotificationPermission() {
        return mNotificationInfo != null ? mNotificationInfo.getContentSetting() : null;
    }

    /**
     * Configure notification setting for this site.
     */
    public void setNotificationPermission(ContentSetting value) {
        if (mNotificationInfo != null) {
            mNotificationInfo.setContentSetting(value);
        }
    }

    /**
     * Sets the Popup exception info for this Website.
     */
    public void setPopupException(ContentSettingException exception) {
        mPopupException = exception;
    }

    public ContentSettingException getPopupException() {
        return mPopupException;
    }

    /**
     * Returns what permission governs Popup permission.
     */
    public ContentSetting getPopupPermission() {
        if (mPopupException != null) return mPopupException.getContentSetting();
        return null;
    }

    /**
     * Configure Popup permission access setting for this site.
     */
    public void setPopupPermission(ContentSetting value) {
        if (mPopupException != null) {
            mPopupException.setContentSetting(value);
        }
    }

    /**
     * Sets protected media identifier access permission information class.
     */
    public void setProtectedMediaIdentifierInfo(ProtectedMediaIdentifierInfo info) {
        mProtectedMediaIdentifierInfo = info;
    }

    public ProtectedMediaIdentifierInfo getProtectedMediaIdentifierInfo() {
        return mProtectedMediaIdentifierInfo;
    }

    /**
     * Returns what permission governs Protected Media Identifier access.
     */
    public ContentSetting getProtectedMediaIdentifierPermission() {
        return mProtectedMediaIdentifierInfo != null
                ? mProtectedMediaIdentifierInfo.getContentSetting() : null;
    }

    /**
     * Configure Protected Media Identifier access setting for this site.
     */
    public void setProtectedMediaIdentifierPermission(ContentSetting value) {
        if (mProtectedMediaIdentifierInfo != null) {
            mProtectedMediaIdentifierInfo.setContentSetting(value);
        }
    }

    public void setLocalStorageInfo(LocalStorageInfo info) {
        mLocalStorageInfo = info;
    }

    public LocalStorageInfo getLocalStorageInfo() {
        return mLocalStorageInfo;
    }

    public void addStorageInfo(StorageInfo info) {
        mStorageInfo.add(info);
    }

    public List<StorageInfo> getStorageInfo() {
        return new ArrayList<StorageInfo>(mStorageInfo);
    }

    public void clearAllStoredData(final StoredDataClearedCallback callback) {
        // Wait for callbacks from each mStorageInfo and another callback from mLocalStorageInfo.
        mStorageInfoCallbacksLeft = mStorageInfo.size() + 1;
        StorageInfoClearedCallback clearedCallback = () -> {
            if (--mStorageInfoCallbacksLeft == 0) callback.onStoredDataCleared();
        };
        if (mLocalStorageInfo != null) {
            mLocalStorageInfo.clear(clearedCallback);
            mLocalStorageInfo = null;
        } else {
            clearedCallback.onStorageInfoCleared();
        }
        for (StorageInfo info : mStorageInfo) {
            info.clear(clearedCallback);
        }
        mStorageInfo.clear();
    }

    /**
     * An interface to implement to get a callback when storage info has been cleared.
     */
    public interface StoredDataClearedCallback {
        public void onStoredDataCleared();
    }

    public long getTotalUsage() {
        long usage = 0;
        if (mLocalStorageInfo != null) {
            usage += mLocalStorageInfo.getSize();
        }
        for (StorageInfo info : mStorageInfo) {
            usage += info.getSize();
        }
        return usage;
    }

    /**
     * Add information about an object the user has granted permission for this site to access.
     */
    public void addChosenObjectInfo(ChosenObjectInfo info) {
        mObjectInfo.add(info);
    }

    /**
     * Returns the set of objects this website has been granted permission to access.
     */
    public List<ChosenObjectInfo> getChosenObjectInfo() {
        return new ArrayList<ChosenObjectInfo>(mObjectInfo);
    }
}
