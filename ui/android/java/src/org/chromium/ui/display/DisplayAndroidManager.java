// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.display;

import android.annotation.SuppressLint;
import android.content.ComponentCallbacks;
import android.content.Context;
import android.content.res.Configuration;
import android.hardware.display.DisplayManager;
import android.hardware.display.DisplayManager.DisplayListener;
import android.os.Build;
import android.util.SparseArray;
import android.view.Display;
import android.view.WindowManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.MainDex;

/**
 * DisplayAndroidManager is a class that informs its observers Display changes.
 */
@JNINamespace("ui")
@MainDex
public class DisplayAndroidManager {
    /**
     * DisplayListenerBackend is an interface that abstract the mechanism used for the actual
     * display update listening. The reason being that from Android API Level 17 DisplayListener
     * will be used. Before that, an unreliable solution based on onConfigurationChanged has to be
     * used.
     */
    private interface DisplayListenerBackend {

        /**
         * Starts to listen for display changes. This will be called
         * when the first observer is added.
         */
        void startListening();

        /**
         * Toggle the accurate mode if it wasn't already doing so. The backend
         * will keep track of the number of times this has been called.
         */
        void startAccurateListening();

        /**
         * Request to stop the accurate mode. It will effectively be stopped
         * only if this method is called as many times as
         * startAccurateListening().
         */
        void stopAccurateListening();
    }

    /**
     * DisplayListenerAPI16 implements DisplayListenerBackend
     * to use ComponentCallbacks in order to listen for display
     * changes.
     *
     * This method is known to not correctly detect 180 degrees changes but it
     * is the only method that will work before API Level 17 (excluding polling).
     * When toggleAccurateMode() is called, it will start polling in order to
     * find out if the display has changed.
     */
    private class DisplayListenerAPI16
            implements DisplayListenerBackend, ComponentCallbacks {

        private static final long POLLING_DELAY = 500;

        private int mAccurateCount;

        // DisplayListenerBackend implementation:

        @Override
        public void startListening() {
            getContext().registerComponentCallbacks(this);
        }

        @Override
        public void startAccurateListening() {
            ++mAccurateCount;

            if (mAccurateCount > 1) return;

            // Start polling if we went from 0 to 1. The polling will
            // automatically stop when mAccurateCount reaches 0.
            final DisplayListenerAPI16 self = this;
            ThreadUtils.postOnUiThreadDelayed(new Runnable() {
                @Override
                public void run() {
                    self.onConfigurationChanged(null);

                    if (self.mAccurateCount < 1) return;

                    ThreadUtils.postOnUiThreadDelayed(this,
                            DisplayListenerAPI16.POLLING_DELAY);
                }
            }, POLLING_DELAY);
        }

        @Override
        public void stopAccurateListening() {
            --mAccurateCount;
            assert mAccurateCount >= 0;
        }

        // ComponentCallbacks implementation:

        @Override
        public void onConfigurationChanged(Configuration newConfig) {
            ((PhysicalDisplayAndroid) mIdMap.get(mMainSdkDisplayId)).updateFromDisplay(
                    getDefaultDisplayForContext(getContext()));
        }

        @Override
        public void onLowMemory() {
        }
    }

    /**
     * DisplayListenerBackendImpl implements DisplayListenerBackend
     * to use DisplayListener in order to listen for display changes.
     *
     * This method is reliable but DisplayListener is only available for API Level 17+.
     */
    @SuppressLint("NewApi")
    private class DisplayListenerBackendImpl
            implements DisplayListenerBackend, DisplayListener {

        // DisplayListenerBackend implementation:

        @Override
        public void startListening() {
            getDisplayManager().registerDisplayListener(this, null);
        }

        @Override
        public void startAccurateListening() {
            // Always accurate. Do nothing.
        }

        @Override
        public void stopAccurateListening() {
            // Always accurate. Do nothing.
        }

        // DisplayListener implementation:

        @Override
        public void onDisplayAdded(int sdkDisplayId) {
            // DisplayAndroid is added lazily on first use. This is to workaround corner case
            // bug where DisplayManager.getDisplay(sdkDisplayId) returning null here.
        }

        @Override
        public void onDisplayRemoved(int sdkDisplayId) {
            // Never remove the primary display.
            if (sdkDisplayId == mMainSdkDisplayId) return;

            DisplayAndroid displayAndroid = mIdMap.get(sdkDisplayId);
            if (displayAndroid == null) return;

            if (mNativePointer != 0) nativeRemoveDisplay(mNativePointer, sdkDisplayId);
            mIdMap.remove(sdkDisplayId);
        }

        @Override
        public void onDisplayChanged(int sdkDisplayId) {
            PhysicalDisplayAndroid displayAndroid =
                    (PhysicalDisplayAndroid) mIdMap.get(sdkDisplayId);
            Display display = getDisplayManager().getDisplay(sdkDisplayId);
            // Note display null check here is needed because there appear to be an edge case in
            // android display code, similar to onDisplayAdded.
            if (displayAndroid != null && display != null) {
                displayAndroid.updateFromDisplay(display);
            }
        }
    }

    private static DisplayAndroidManager sDisplayAndroidManager;

    // Real displays (as in, displays backed by an Android Display and recognized by the OS, though
    // not necessarily physical displays) on Android start at ID 0, and increment indefinitely as
    // displays are added. Display IDs are never reused until reboot. To avoid any overlap, start
    // virtual display ids at a much higher number, and increment them in the same way.
    private static final int VIRTUAL_DISPLAY_ID_BEGIN = Integer.MAX_VALUE / 2;

    private long mNativePointer;
    private int mMainSdkDisplayId;
    private final SparseArray<DisplayAndroid> mIdMap = new SparseArray<>();
    private DisplayListenerBackend mBackend;
    private int mNextVirtualDisplayId = VIRTUAL_DISPLAY_ID_BEGIN;

    /* package */ static DisplayAndroidManager getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sDisplayAndroidManager == null) {
            // Split between creation and initialization to allow for calls from DisplayAndroid to
            // reference sDisplayAndroidManager during initialize().
            sDisplayAndroidManager = new DisplayAndroidManager();
            sDisplayAndroidManager.initialize();
        }
        return sDisplayAndroidManager;
    }

    public static Display getDefaultDisplayForContext(Context context) {
        WindowManager windowManager =
                (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        return windowManager.getDefaultDisplay();
    }

    private static Context getContext() {
        // Using the global application context is probably ok.
        // The DisplayManager API observers all display updates, so in theory it should not matter
        // which context is used to obtain it. If this turns out not to be true in practice, it's
        // possible register from all context used though quite complex.
        return ContextUtils.getApplicationContext();
    }

    private static DisplayManager getDisplayManager() {
        return (DisplayManager) getContext().getSystemService(Context.DISPLAY_SERVICE);
    }

    @CalledByNative
    private static void onNativeSideCreated(long nativePointer) {
        DisplayAndroidManager singleton = getInstance();
        singleton.setNativePointer(nativePointer);
    }

    private DisplayAndroidManager() {}

    private void initialize() {
        Display display;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            mBackend = new DisplayListenerBackendImpl();
            // Make sure the display map contains the built-in primary display.
            // The primary display is never removed.
            display = getDisplayManager().getDisplay(Display.DEFAULT_DISPLAY);

            // Android documentation on Display.DEFAULT_DISPLAY suggests that the above
            // method might return null. In that case we retrieve the default display
            // from the application context and take it as the primary display.
            if (display == null) display = getDefaultDisplayForContext(getContext());
        } else {
            mBackend = new DisplayListenerAPI16();
            display = getDefaultDisplayForContext(getContext());
        }

        mMainSdkDisplayId = display.getDisplayId();
        addDisplay(display); // Note this display is never removed.

        mBackend.startListening();
    }

    private void setNativePointer(long nativePointer) {
        mNativePointer = nativePointer;
        nativeSetPrimaryDisplayId(mNativePointer, mMainSdkDisplayId);

        for (int i = 0; i < mIdMap.size(); ++i) {
            updateDisplayOnNativeSide(mIdMap.valueAt(i));
        }
    }

    /* package */ DisplayAndroid getDisplayAndroid(Display display) {
        int sdkDisplayId = display.getDisplayId();
        DisplayAndroid displayAndroid = mIdMap.get(sdkDisplayId);
        if (displayAndroid == null) {
            displayAndroid = addDisplay(display);
        }
        return displayAndroid;
    }

    /* package */ void startAccurateListening() {
        mBackend.startAccurateListening();
    }

    /* package */ void stopAccurateListening() {
        mBackend.stopAccurateListening();
    }

    private DisplayAndroid addDisplay(Display display) {
        int sdkDisplayId = display.getDisplayId();
        PhysicalDisplayAndroid displayAndroid = new PhysicalDisplayAndroid(display);
        assert mIdMap.get(sdkDisplayId) == null;
        mIdMap.put(sdkDisplayId, displayAndroid);
        displayAndroid.updateFromDisplay(display);
        return displayAndroid;
    }

    private int getNextVirtualDisplayId() {
        return mNextVirtualDisplayId++;
    }

    /* package */ VirtualDisplayAndroid addVirtualDisplay() {
        VirtualDisplayAndroid display = new VirtualDisplayAndroid(getNextVirtualDisplayId());
        assert mIdMap.get(display.getDisplayId()) == null;
        mIdMap.put(display.getDisplayId(), display);
        updateDisplayOnNativeSide(display);
        return display;
    }

    /* package */ void removeVirtualDisplay(VirtualDisplayAndroid display) {
        DisplayAndroid displayAndroid = mIdMap.get(display.getDisplayId());
        assert displayAndroid == display;

        if (mNativePointer != 0) nativeRemoveDisplay(mNativePointer, display.getDisplayId());
        mIdMap.remove(display.getDisplayId());
    }

    /* package */ void updateDisplayOnNativeSide(DisplayAndroid displayAndroid) {
        if (mNativePointer == 0) return;
        nativeUpdateDisplay(mNativePointer, displayAndroid.getDisplayId(),
                displayAndroid.getDisplayWidth(), displayAndroid.getDisplayHeight(),
                displayAndroid.getDipScale(), displayAndroid.getRotationDegrees(),
                displayAndroid.getBitsPerPixel(), displayAndroid.getBitsPerComponent(),
                displayAndroid.getIsWideColorGamut());
    }

    private native void nativeUpdateDisplay(long nativeDisplayAndroidManager, int sdkDisplayId,
            int width, int height, float dipScale, int rotationDegrees, int bitsPerPixel,
            int bitsPerComponent, boolean isWideColorGamut);
    private native void nativeRemoveDisplay(long nativeDisplayAndroidManager, int sdkDisplayId);
    private native void nativeSetPrimaryDisplayId(
            long nativeDisplayAndroidManager, int sdkDisplayId);
}
