// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.usb;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.util.HashMap;

/**
 * Exposes android.hardware.usb.UsbManager as necessary for C++
 * device::UsbServiceAndroid.
 *
 * Lifetime is controlled by device::UsbServiceAndroid.
 */
@JNINamespace("device")
final class ChromeUsbService {
    private static final String TAG = "Usb";
    private static final String ACTION_USB_PERMISSION = "org.chromium.device.ACTION_USB_PERMISSION";

    long mUsbServiceAndroid;
    UsbManager mUsbManager;
    BroadcastReceiver mUsbDeviceReceiver;

    private ChromeUsbService(long usbServiceAndroid) {
        mUsbServiceAndroid = usbServiceAndroid;
        mUsbManager = (UsbManager) ContextUtils.getApplicationContext().getSystemService(
                Context.USB_SERVICE);
        registerForUsbDeviceIntentBroadcast();
        Log.v(TAG, "ChromeUsbService created.");
    }

    @CalledByNative
    private static ChromeUsbService create(long usbServiceAndroid) {
        return new ChromeUsbService(usbServiceAndroid);
    }

    @CalledByNative
    private Object[] getDevices() {
        HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
        return deviceList.values().toArray();
    }

    @CalledByNative
    private UsbDeviceConnection openDevice(ChromeUsbDevice wrapper) {
        UsbDevice device = wrapper.getDevice();
        return mUsbManager.openDevice(device);
    }

    @CalledByNative
    private void requestDevicePermission(ChromeUsbDevice wrapper, long nativeCallback) {
        UsbDevice device = wrapper.getDevice();
        if (mUsbManager.hasPermission(device)) {
            nativeDevicePermissionRequestComplete(mUsbServiceAndroid, device.getDeviceId(), true);
        } else {
            PendingIntent intent = PendingIntent.getBroadcast(
                    ContextUtils.getApplicationContext(), 0, new Intent(ACTION_USB_PERMISSION), 0);
            mUsbManager.requestPermission(wrapper.getDevice(), intent);
        }
    }

    @CalledByNative
    private void close() {
        unregisterForUsbDeviceIntentBroadcast();
    }

    private native void nativeDeviceAttached(long nativeUsbServiceAndroid, UsbDevice device);

    private native void nativeDeviceDetached(long nativeUsbServiceAndroid, int deviceId);

    private native void nativeDevicePermissionRequestComplete(
            long nativeUsbServiceAndroid, int deviceId, boolean granted);

    private void registerForUsbDeviceIntentBroadcast() {
        mUsbDeviceReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(intent.getAction())) {
                    nativeDeviceAttached(mUsbServiceAndroid, device);
                } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(intent.getAction())) {
                    nativeDeviceDetached(mUsbServiceAndroid, device.getDeviceId());
                } else if (ACTION_USB_PERMISSION.equals(intent.getAction())) {
                    nativeDevicePermissionRequestComplete(mUsbServiceAndroid, device.getDeviceId(),
                            intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false));
                }
            }
        };

        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(ACTION_USB_PERMISSION);
        ContextUtils.getApplicationContext().registerReceiver(mUsbDeviceReceiver, filter);
    }

    private void unregisterForUsbDeviceIntentBroadcast() {
        ContextUtils.getApplicationContext().unregisterReceiver(mUsbDeviceReceiver);
        mUsbDeviceReceiver = null;
    }
}
