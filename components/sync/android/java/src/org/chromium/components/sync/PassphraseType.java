// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.sync;

import android.os.Parcel;
import android.os.Parcelable;

import java.util.HashSet;
import java.util.Set;

/**
 * This enum describes the type of passphrase required, if any, to decrypt synced data.
 *
 * It implements the Android {@link Parcelable} interface so it is easy to pass around in intents.
 *
 * It maps the native enum PassphraseType.
 */
public enum PassphraseType implements Parcelable {
    IMPLICIT_PASSPHRASE(0), // GAIA-based passphrase (deprecated).
    KEYSTORE_PASSPHRASE(1), // Keystore passphrase.
    FROZEN_IMPLICIT_PASSPHRASE(2), // Frozen GAIA passphrase.
    CUSTOM_PASSPHRASE(3); // User-provided passphrase.

    public static Parcelable.Creator CREATOR = new Parcelable.Creator<PassphraseType>() {
        @Override
        public PassphraseType createFromParcel(Parcel parcel) {
            return fromInternalValue(parcel.readInt());
        }

        @Override
        public PassphraseType[] newArray(int size) {
            return new PassphraseType[size];
        }
    };

    public static PassphraseType fromInternalValue(int value) {
        for (PassphraseType type : values()) {
            if (type.internalValue() == value) {
                return type;
            }
        }
        throw new IllegalArgumentException("No value for " + value + " found.");
    }

    private final int mNativeValue;

    private PassphraseType(int nativeValue) {
        mNativeValue = nativeValue;
    }

    public Set<PassphraseType> getVisibleTypes() {
        Set<PassphraseType> visibleTypes = new HashSet<>();
        switch (this) {
            case IMPLICIT_PASSPHRASE: // Intentional fall through.
            case KEYSTORE_PASSPHRASE:
                visibleTypes.add(this);
                visibleTypes.add(CUSTOM_PASSPHRASE);
                break;
            case FROZEN_IMPLICIT_PASSPHRASE:
                visibleTypes.add(KEYSTORE_PASSPHRASE);
                visibleTypes.add(FROZEN_IMPLICIT_PASSPHRASE);
                break;
            case CUSTOM_PASSPHRASE:
                visibleTypes.add(KEYSTORE_PASSPHRASE);
                visibleTypes.add(CUSTOM_PASSPHRASE);
                break;
        }
        return visibleTypes;
    }

    /**
     * Get the types that are allowed to be enabled from the current type.
     *
     * @param isEncryptEverythingAllowed Whether encrypting all data is allowed.
     */
    public Set<PassphraseType> getAllowedTypes(boolean isEncryptEverythingAllowed) {
        Set<PassphraseType> allowedTypes = new HashSet<>();
        switch (this) {
            case IMPLICIT_PASSPHRASE: // Intentional fall through.
            case KEYSTORE_PASSPHRASE:
                allowedTypes.add(this);
                if (isEncryptEverythingAllowed) {
                    allowedTypes.add(CUSTOM_PASSPHRASE);
                }
                break;
            case FROZEN_IMPLICIT_PASSPHRASE: // Intentional fall through.
            case CUSTOM_PASSPHRASE: // Intentional fall through.
            default:
                break;
        }
        return allowedTypes;
    }

    public int internalValue() {
        // Since the values in this enums are constant and very small, this cast is safe.
        return mNativeValue;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mNativeValue);
    }
}
