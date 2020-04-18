// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.website;

import org.chromium.base.Callback;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.ContentSettingsType;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * Utility class that interacts with native to retrieve and set website settings.
 */
public abstract class WebsitePreferenceBridge {
    private static final String LOG_TAG = "WebsiteSettingsUtils";

    /**
     * Interface for an object that listens to storage info is cleared callback.
     */
    public interface StorageInfoClearedCallback {
        @CalledByNative("StorageInfoClearedCallback")
        public void onStorageInfoCleared();
    }

    /**
     * @return the list of all origins that have clipboard permissions in non-incognito mode.
     */
    @SuppressWarnings("unchecked")
    public static List<ClipboardInfo> getClipboardInfo() {
        ArrayList<ClipboardInfo> list = new ArrayList<ClipboardInfo>();
        nativeGetClipboardOrigins(list);
        return list;
    }

    @CalledByNative
    private static void insertClipboardInfoIntoList(
            ArrayList<ClipboardInfo> list, String origin, String embedder) {
        list.add(new ClipboardInfo(origin, embedder, false));
    }

    /**
     * @return the list of all origins that have geolocation permissions in non-incognito mode.
     */
    @SuppressWarnings("unchecked")
    public static List<GeolocationInfo> getGeolocationInfo() {
        // Location can be managed by the custodian of a supervised account or by enterprise policy.
        boolean managedOnly = !PrefServiceBridge.getInstance().isAllowLocationUserModifiable();
        ArrayList<GeolocationInfo> list = new ArrayList<GeolocationInfo>();
        nativeGetGeolocationOrigins(list, managedOnly);
        return list;
    }

    @CalledByNative
    private static void insertGeolocationInfoIntoList(
            ArrayList<GeolocationInfo> list, String origin, String embedder) {
        list.add(new GeolocationInfo(origin, embedder, false));
    }

    /**
     * @return the list of all origins that have midi permissions in non-incognito mode.
     */
    @SuppressWarnings("unchecked")
    public static List<MidiInfo> getMidiInfo() {
        ArrayList<MidiInfo> list = new ArrayList<MidiInfo>();
        nativeGetMidiOrigins(list);
        return list;
    }

    @CalledByNative
    private static void insertMidiInfoIntoList(
            ArrayList<MidiInfo> list, String origin, String embedder) {
        list.add(new MidiInfo(origin, embedder, false));
    }

    @CalledByNative
    private static Object createStorageInfoList() {
        return new ArrayList<StorageInfo>();
    }

    @CalledByNative
    private static void insertStorageInfoIntoList(
            ArrayList<StorageInfo> list, String host, int type, long size) {
        list.add(new StorageInfo(host, type, size));
    }

    @CalledByNative
    private static Object createLocalStorageInfoMap() {
        return new HashMap<String, LocalStorageInfo>();
    }

    @SuppressWarnings("unchecked")
    @CalledByNative
    private static void insertLocalStorageInfoIntoMap(
            HashMap map, String origin, String fullOrigin, long size, boolean important) {
        ((HashMap<String, LocalStorageInfo>) map)
                .put(origin, new LocalStorageInfo(origin, size, important));
    }

    /**
     * @return the list of all origins that have protected media identifier permissions
     *         in non-incognito mode.
     */
    @SuppressWarnings("unchecked")
    public static List<ProtectedMediaIdentifierInfo> getProtectedMediaIdentifierInfo() {
        ArrayList<ProtectedMediaIdentifierInfo> list =
                new ArrayList<ProtectedMediaIdentifierInfo>();
        nativeGetProtectedMediaIdentifierOrigins(list);
        return list;
    }

    @CalledByNative
    private static void insertProtectedMediaIdentifierInfoIntoList(
            ArrayList<ProtectedMediaIdentifierInfo> list, String origin, String embedder) {
        list.add(new ProtectedMediaIdentifierInfo(origin, embedder, false));
    }

    /**
     * @return the list of all origins that have notification permissions in non-incognito mode.
     */
    @SuppressWarnings("unchecked")
    public static List<NotificationInfo> getNotificationInfo() {
        ArrayList<NotificationInfo> list = new ArrayList<NotificationInfo>();
        nativeGetNotificationOrigins(list);
        return list;
    }

    @CalledByNative
    private static void insertNotificationIntoList(
            ArrayList<NotificationInfo> list, String origin, String embedder) {
        list.add(new NotificationInfo(origin, embedder, false));
    }

    /**
     * @return the list of all origins that have camera permissions in non-incognito mode.
     */
    @SuppressWarnings("unchecked")
    public static List<CameraInfo> getCameraInfo() {
        ArrayList<CameraInfo> list = new ArrayList<CameraInfo>();
        // Camera can be managed by the custodian of a supervised account or by enterprise policy.
        boolean managedOnly = !PrefServiceBridge.getInstance().isCameraUserModifiable();
        nativeGetCameraOrigins(list, managedOnly);
        return list;
    }

    @CalledByNative
    private static void insertCameraInfoIntoList(
            ArrayList<CameraInfo> list, String origin, String embedder) {
        for (int i = 0; i < list.size(); i++) {
            if (list.get(i).getOrigin().equals(origin)
                        && list.get(i).getEmbedder().equals(embedder)) {
                return;
            }
        }
        list.add(new CameraInfo(origin, embedder, false));
    }

    /**
     * @return the list of all origins that have microphone permissions in non-incognito mode.
     */
    @SuppressWarnings("unchecked")
    public static List<MicrophoneInfo> getMicrophoneInfo() {
        ArrayList<MicrophoneInfo> list =
                new ArrayList<MicrophoneInfo>();
        // Microphone can be managed by the custodian of a supervised account or by enterprise
        // policy.
        boolean managedOnly = !PrefServiceBridge.getInstance().isMicUserModifiable();
        nativeGetMicrophoneOrigins(list, managedOnly);
        return list;
    }

    @CalledByNative
    private static void insertMicrophoneInfoIntoList(
            ArrayList<MicrophoneInfo> list, String origin, String embedder) {
        for (int i = 0; i < list.size(); i++) {
            if (list.get(i).getOrigin().equals(origin)
                        && list.get(i).getEmbedder().equals(embedder)) {
                return;
            }
        }
        list.add(new MicrophoneInfo(origin, embedder, false));
    }

    public static List<ContentSettingException> getContentSettingsExceptions(
            @ContentSettingsType int contentSettingsType) {
        List<ContentSettingException> exceptions =
                PrefServiceBridge.getInstance().getContentSettingsExceptions(
                        contentSettingsType);
        if (!PrefServiceBridge.getInstance().isContentSettingManaged(contentSettingsType)) {
            return exceptions;
        }

        List<ContentSettingException> managedExceptions =
                new ArrayList<ContentSettingException>();
        for (ContentSettingException exception : exceptions) {
            if (exception.getSource().equals("policy")) {
                managedExceptions.add(exception);
            }
        }
        return managedExceptions;
    }

    public static void fetchLocalStorageInfo(Callback<HashMap> callback, boolean fetchImportant) {
        nativeFetchLocalStorageInfo(callback, fetchImportant);
    }

    public static void fetchStorageInfo(Callback<ArrayList> callback) {
        nativeFetchStorageInfo(callback);
    }

    /**
     * Returns the list of all chosen object permissions for the given ContentSettingsType.
     *
     * There will be one ChosenObjectInfo instance for each granted permission. That means that if
     * two origin/embedder pairs have permission for the same object there will be two
     * ChosenObjectInfo instances.
     */
    public static List<ChosenObjectInfo> getChosenObjectInfo(
            @ContentSettingsType int contentSettingsType) {
        ArrayList<ChosenObjectInfo> list = new ArrayList<ChosenObjectInfo>();
        nativeGetChosenObjects(contentSettingsType, list);
        return list;
    }

    /**
     * Inserts a ChosenObjectInfo into a list.
     */
    @CalledByNative
    private static void insertChosenObjectInfoIntoList(ArrayList<ChosenObjectInfo> list,
            int contentSettingsType, String origin, String embedder, String name, String object) {
        list.add(new ChosenObjectInfo(contentSettingsType, origin, embedder, name, object));
    }

    /**
     * Returns whether the DSE (Default Search Engine) controls the given permission the given
     * origin.
     */
    public static boolean isPermissionControlledByDSE(
            @ContentSettingsType int contentSettingsType, String origin, boolean isIncognito) {
        return nativeIsPermissionControlledByDSE(contentSettingsType, origin, isIncognito);
    }

    /**
     * Returns whether this origin is activated for ad blocking, and will have resources blocked
     * unless they are explicitly allowed via a permission.
     */
    public static boolean getAdBlockingActivated(String origin) {
        return nativeGetAdBlockingActivated(origin);
    }

    private static native void nativeGetClipboardOrigins(Object list);
    static native int nativeGetClipboardSettingForOrigin(
            String origin, boolean isIncognito);
    public static native void nativeSetClipboardSettingForOrigin(
            String origin, int value, boolean isIncognito);
    private static native void nativeGetGeolocationOrigins(Object list, boolean managedOnly);
    static native int nativeGetGeolocationSettingForOrigin(
            String origin, String embedder, boolean isIncognito);
    public static native void nativeSetGeolocationSettingForOrigin(
            String origin, String embedder, int value, boolean isIncognito);
    private static native void nativeGetMidiOrigins(Object list);
    static native int nativeGetMidiSettingForOrigin(
            String origin, String embedder, boolean isIncognito);
    static native void nativeSetMidiSettingForOrigin(
            String origin, String embedder, int value, boolean isIncognito);
    private static native void nativeGetNotificationOrigins(Object list);
    static native int nativeGetNotificationSettingForOrigin(
            String origin, boolean isIncognito);
    static native void nativeSetNotificationSettingForOrigin(
            String origin, int value, boolean isIncognito);
    private static native void nativeGetProtectedMediaIdentifierOrigins(Object list);
    static native int nativeGetProtectedMediaIdentifierSettingForOrigin(
            String origin, String embedder, boolean isIncognito);
    static native void nativeSetProtectedMediaIdentifierSettingForOrigin(
            String origin, String embedder, int value, boolean isIncognito);
    private static native void nativeGetCameraOrigins(Object list, boolean managedOnly);
    private static native void nativeGetMicrophoneOrigins(Object list, boolean managedOnly);
    static native int nativeGetMicrophoneSettingForOrigin(
            String origin, String embedder, boolean isIncognito);
    static native int nativeGetCameraSettingForOrigin(
            String origin, String embedder, boolean isIncognito);
    static native void nativeSetMicrophoneSettingForOrigin(
            String origin, int value, boolean isIncognito);
    static native void nativeSetCameraSettingForOrigin(
            String origin, int value, boolean isIncognito);
    static native void nativeClearCookieData(String path);
    static native void nativeClearLocalStorageData(String path, Object callback);
    static native void nativeClearStorageData(String origin, int type, Object callback);
    private static native void nativeFetchLocalStorageInfo(
            Object callback, boolean includeImportant);
    private static native void nativeFetchStorageInfo(Object callback);
    static native boolean nativeIsContentSettingsPatternValid(String pattern);
    static native boolean nativeUrlMatchesContentSettingsPattern(String url, String pattern);
    static native void nativeGetChosenObjects(@ContentSettingsType int type, Object list);
    static native void nativeRevokeObjectPermission(
            @ContentSettingsType int type, String origin, String embedder, String object);
    static native void nativeClearBannerData(String origin);
    private static native boolean nativeIsPermissionControlledByDSE(
            @ContentSettingsType int contentSettingsType, String origin, boolean isIncognito);
    private static native boolean nativeGetAdBlockingActivated(String origin);
    static native void nativeResetNotificationsSettingsForTest();
}
