// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.bluetooth;

import android.annotation.TargetApi;
import android.os.Build;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNIAdditionalImport;
import org.chromium.base.annotations.JNINamespace;

import java.util.List;

/**
 * Exposes android.bluetooth.BluetoothGattCharacteristic as necessary
 * for C++ device::BluetoothRemoteGattCharacteristicAndroid.
 *
 * Lifetime is controlled by
 * device::BluetoothRemoteGattCharacteristicAndroid.
 */
@JNINamespace("device")
@JNIAdditionalImport(Wrappers.class)
@TargetApi(Build.VERSION_CODES.M)
final class ChromeBluetoothRemoteGattCharacteristic {
    private static final String TAG = "Bluetooth";

    private long mNativeBluetoothRemoteGattCharacteristicAndroid;
    final Wrappers.BluetoothGattCharacteristicWrapper mCharacteristic;
    final String mInstanceId;
    final ChromeBluetoothDevice mChromeDevice;

    private ChromeBluetoothRemoteGattCharacteristic(
            long nativeBluetoothRemoteGattCharacteristicAndroid,
            Wrappers.BluetoothGattCharacteristicWrapper characteristicWrapper, String instanceId,
            ChromeBluetoothDevice chromeDevice) {
        mNativeBluetoothRemoteGattCharacteristicAndroid =
                nativeBluetoothRemoteGattCharacteristicAndroid;
        mCharacteristic = characteristicWrapper;
        mInstanceId = instanceId;
        mChromeDevice = chromeDevice;

        mChromeDevice.mWrapperToChromeCharacteristicsMap.put(characteristicWrapper, this);

        Log.v(TAG, "ChromeBluetoothRemoteGattCharacteristic created.");
    }

    /**
     * Handles C++ object being destroyed.
     */
    @CalledByNative
    private void onBluetoothRemoteGattCharacteristicAndroidDestruction() {
        Log.v(TAG, "ChromeBluetoothRemoteGattCharacteristic Destroyed.");
        if (mChromeDevice.mBluetoothGatt != null) {
            mChromeDevice.mBluetoothGatt.setCharacteristicNotification(mCharacteristic, false);
        }
        mNativeBluetoothRemoteGattCharacteristicAndroid = 0;
        mChromeDevice.mWrapperToChromeCharacteristicsMap.remove(mCharacteristic);
    }

    void onCharacteristicChanged(byte[] value) {
        Log.i(TAG, "onCharacteristicChanged");
        if (mNativeBluetoothRemoteGattCharacteristicAndroid != 0) {
            nativeOnChanged(mNativeBluetoothRemoteGattCharacteristicAndroid, value);
        }
    }

    void onCharacteristicRead(int status) {
        Log.i(TAG, "onCharacteristicRead status:%d==%s", status,
                status == android.bluetooth.BluetoothGatt.GATT_SUCCESS ? "OK" : "Error");
        if (mNativeBluetoothRemoteGattCharacteristicAndroid != 0) {
            nativeOnRead(mNativeBluetoothRemoteGattCharacteristicAndroid, status,
                    mCharacteristic.getValue());
        }
    }

    void onCharacteristicWrite(int status) {
        Log.i(TAG, "onCharacteristicWrite status:%d==%s", status,
                status == android.bluetooth.BluetoothGatt.GATT_SUCCESS ? "OK" : "Error");
        if (mNativeBluetoothRemoteGattCharacteristicAndroid != 0) {
            nativeOnWrite(mNativeBluetoothRemoteGattCharacteristicAndroid, status);
        }
    }

    // ---------------------------------------------------------------------------------------------
    // BluetoothRemoteGattCharacteristicAndroid methods implemented in java:

    // Implements BluetoothRemoteGattCharacteristicAndroid::Create.
    @CalledByNative
    private static ChromeBluetoothRemoteGattCharacteristic create(
            long nativeBluetoothRemoteGattCharacteristicAndroid,
            Wrappers.BluetoothGattCharacteristicWrapper characteristicWrapper, String instanceId,
            ChromeBluetoothDevice chromeDevice) {
        return new ChromeBluetoothRemoteGattCharacteristic(
                nativeBluetoothRemoteGattCharacteristicAndroid, characteristicWrapper, instanceId,
                chromeDevice);
    }

    // Implements BluetoothRemoteGattCharacteristicAndroid::GetUUID.
    @CalledByNative
    private String getUUID() {
        return mCharacteristic.getUuid().toString();
    }

    // Implements BluetoothRemoteGattCharacteristicAndroid::GetProperties.
    @CalledByNative
    private int getProperties() {
        // TODO(scheib): Must read Extended Properties Descriptor. crbug.com/548449
        return mCharacteristic.getProperties();
    }

    // Implements BluetoothRemoteGattCharacteristicAndroid::ReadRemoteCharacteristic.
    @CalledByNative
    private boolean readRemoteCharacteristic() {
        if (!mChromeDevice.mBluetoothGatt.readCharacteristic(mCharacteristic)) {
            Log.i(TAG, "readRemoteCharacteristic readCharacteristic failed.");
            return false;
        }
        return true;
    }

    // Implements BluetoothRemoteGattCharacteristicAndroid::WriteRemoteCharacteristic.
    @CalledByNative
    private boolean writeRemoteCharacteristic(byte[] value) {
        if (!mCharacteristic.setValue(value)) {
            Log.i(TAG, "writeRemoteCharacteristic setValue failed.");
            return false;
        }
        if (!mChromeDevice.mBluetoothGatt.writeCharacteristic(mCharacteristic)) {
            Log.i(TAG, "writeRemoteCharacteristic writeCharacteristic failed.");
            return false;
        }
        return true;
    }

    // Enable or disable the notifications for this characteristic.
    @CalledByNative
    private boolean setCharacteristicNotification(boolean enabled) {
        return mChromeDevice.mBluetoothGatt.setCharacteristicNotification(mCharacteristic, enabled);
    }

    // Creates objects for all descriptors. Designed only to be called by
    // BluetoothRemoteGattCharacteristicAndroid::EnsureDescriptorsCreated.
    @CalledByNative
    private void createDescriptors() {
        List<Wrappers.BluetoothGattDescriptorWrapper> descriptors =
                mCharacteristic.getDescriptors();
        // descriptorInstanceId ensures duplicate UUIDs have unique instance
        // IDs. BluetoothGattDescriptor does not offer getInstanceId the way
        // BluetoothGattCharacteristic does.
        //
        // TODO(crbug.com/576906) Do not reuse IDs upon onServicesDiscovered.
        int instanceIdCounter = 0;
        for (Wrappers.BluetoothGattDescriptorWrapper descriptor : descriptors) {
            String descriptorInstanceId =
                    mInstanceId + "/" + descriptor.getUuid().toString() + ";" + instanceIdCounter++;
            nativeCreateGattRemoteDescriptor(mNativeBluetoothRemoteGattCharacteristicAndroid,
                    descriptorInstanceId, descriptor, mChromeDevice);
        }
    }

    // ---------------------------------------------------------------------------------------------
    // BluetoothAdapterDevice C++ methods declared for access from java:

    // Binds to BluetoothRemoteGattCharacteristicAndroid::OnChanged.
    native void nativeOnChanged(long nativeBluetoothRemoteGattCharacteristicAndroid, byte[] value);

    // Binds to BluetoothRemoteGattCharacteristicAndroid::OnRead.
    native void nativeOnRead(
            long nativeBluetoothRemoteGattCharacteristicAndroid, int status, byte[] value);

    // Binds to BluetoothRemoteGattCharacteristicAndroid::OnWrite.
    native void nativeOnWrite(long nativeBluetoothRemoteGattCharacteristicAndroid, int status);

    // Binds to BluetoothRemoteGattCharacteristicAndroid::CreateGattRemoteDescriptor.
    private native void nativeCreateGattRemoteDescriptor(
            long nativeBluetoothRemoteGattCharacteristicAndroid, String instanceId,
            Wrappers.BluetoothGattDescriptorWrapper descriptorWrapper,
            ChromeBluetoothDevice chromeBluetoothDevice);
}
