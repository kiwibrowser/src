// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.dom_distiller.core;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeCall;

import java.util.HashMap;
import java.util.Map;

/**
 * Wrapper for the dom_distiller::DistilledPagePrefs.
 */
@JNINamespace("dom_distiller::android")
public final class DistilledPagePrefs {

    private final long mDistilledPagePrefsAndroid;
    private Map<Observer, DistilledPagePrefsObserverWrapper> mObserverMap;

    /**
     * Observer interface for observing DistilledPagePrefs changes.
     */
    public interface Observer {
        void onChangeFontFamily(FontFamily font);
        void onChangeTheme(Theme theme);
        void onChangeFontScaling(float scaling);
    }

    /**
     * Wrapper for dom_distiller::android::DistilledPagePrefsObserverAndroid.
     */
    private static class DistilledPagePrefsObserverWrapper {
        private final Observer mDistilledPagePrefsObserver;
        private final long mNativeDistilledPagePrefsObserverAndroidPtr;

        public DistilledPagePrefsObserverWrapper(Observer observer) {
            mNativeDistilledPagePrefsObserverAndroidPtr = nativeInitObserverAndroid();
            mDistilledPagePrefsObserver = observer;
        }

        @CalledByNative("DistilledPagePrefsObserverWrapper")
        private void onChangeFontFamily(int fontFamily) {
            mDistilledPagePrefsObserver.onChangeFontFamily(
                    FontFamily.getFontFamilyForValue(fontFamily));
        }

        @CalledByNative("DistilledPagePrefsObserverWrapper")
        private void onChangeTheme(int theme) {
            mDistilledPagePrefsObserver.onChangeTheme(Theme.getThemeForValue(theme));
        }

        @CalledByNative("DistilledPagePrefsObserverWrapper")
        private void onChangeFontScaling(float scaling) {
            mDistilledPagePrefsObserver.onChangeFontScaling(scaling);
        }

        public void destroy() {
            nativeDestroyObserverAndroid(mNativeDistilledPagePrefsObserverAndroidPtr);
        }

        public long getNativePtr() {
            return mNativeDistilledPagePrefsObserverAndroidPtr;
        }

        @NativeCall("DistilledPagePrefsObserverWrapper")
        private native long nativeInitObserverAndroid();

        @NativeCall("DistilledPagePrefsObserverWrapper")
        private native void nativeDestroyObserverAndroid(
                long nativeDistilledPagePrefsObserverAndroid);
    }

    DistilledPagePrefs(long distilledPagePrefsPtr) {
        mDistilledPagePrefsAndroid = nativeInit(distilledPagePrefsPtr);
        mObserverMap = new HashMap<Observer, DistilledPagePrefsObserverWrapper>();
    }

    /*
     * Adds the observer to listen to changes in DistilledPagePrefs.
     * @return whether the observerMap was changed as a result of the call.
     */
    public boolean addObserver(Observer obs) {
        if (mObserverMap.containsKey(obs)) return false;
        DistilledPagePrefsObserverWrapper wrappedObserver =
                new DistilledPagePrefsObserverWrapper(obs);
        nativeAddObserver(mDistilledPagePrefsAndroid, wrappedObserver.getNativePtr());
        mObserverMap.put(obs, wrappedObserver);
        return true;
    }

    /*
     * Removes the observer and unregisters it from DistilledPagePrefs changes.
     * @return whether the observer was removed as a result of the call.
     */
    public boolean removeObserver(Observer obs) {
        DistilledPagePrefsObserverWrapper wrappedObserver = mObserverMap.remove(obs);
        if (wrappedObserver == null) return false;
        nativeRemoveObserver(mDistilledPagePrefsAndroid, wrappedObserver.getNativePtr());
        wrappedObserver.destroy();
        return true;
    }

    public void setFontFamily(FontFamily fontFamily) {
        nativeSetFontFamily(mDistilledPagePrefsAndroid, fontFamily.asNativeEnum());
    }

    public FontFamily getFontFamily() {
        return FontFamily.getFontFamilyForValue(nativeGetFontFamily(mDistilledPagePrefsAndroid));
    }

    public void setTheme(Theme theme) {
        nativeSetTheme(mDistilledPagePrefsAndroid, theme.asNativeEnum());
    }

    public Theme getTheme() {
        return Theme.getThemeForValue(nativeGetTheme(mDistilledPagePrefsAndroid));
    }

    public void setFontScaling(float scaling) {
        nativeSetFontScaling(mDistilledPagePrefsAndroid, scaling);
    }

    public float getFontScaling() {
        return nativeGetFontScaling(mDistilledPagePrefsAndroid);
    }

    private native long nativeInit(long distilledPagePrefPtr);

    private native void nativeSetFontFamily(long nativeDistilledPagePrefsAndroid, int fontFamily);

    private native int nativeGetFontFamily(long nativeDistilledPagePrefsAndroid);

    private native void nativeSetTheme(long nativeDistilledPagePrefsAndroid, int theme);

    private native int nativeGetTheme(long nativeDistilledPagePrefsAndroid);

    private native void nativeSetFontScaling(long nativeDistilledPagePrefsAndroid, float scaling);

    private native float nativeGetFontScaling(long nativeDistilledPagePrefsAndroid);

    private native void nativeAddObserver(long nativeDistilledPagePrefsAndroid,
            long nativeObserverPtr);

    private native void nativeRemoveObserver(long nativeDistilledPagePrefsAndroid,
            long nativeObserverPtr);
}
