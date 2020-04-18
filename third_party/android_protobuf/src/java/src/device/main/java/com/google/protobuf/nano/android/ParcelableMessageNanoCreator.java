// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf.nano.android;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.google.protobuf.nano.InvalidProtocolBufferNanoException;
import com.google.protobuf.nano.MessageNano;

import java.lang.reflect.Array;
import java.lang.reflect.InvocationTargetException;

public final class ParcelableMessageNanoCreator<T extends MessageNano>
        implements Parcelable.Creator<T> {
    private static final String TAG = "PMNCreator";

    private final Class<T> mClazz;

    public ParcelableMessageNanoCreator(Class<T> clazz) {
        mClazz = clazz;
    }

    @SuppressWarnings("unchecked")
    @Override
    public T createFromParcel(Parcel in) {
        String className = in.readString();
        byte[] data = in.createByteArray();

        T proto = null;

        try {
            // Check that the provided class is a subclass of MessageNano before executing any code
            Class<?> clazz =
                Class.forName(className, false /*initialize*/, this.getClass().getClassLoader())
                    .asSubclass(MessageNano.class);
            Object instance = clazz.getConstructor().newInstance();
            proto = (T) instance;
            MessageNano.mergeFrom(proto, data);
        } catch (ClassNotFoundException e) {
            Log.e(TAG, "Exception trying to create proto from parcel", e);
        } catch (NoSuchMethodException e) {
            Log.e(TAG, "Exception trying to create proto from parcel", e);
        } catch (InvocationTargetException e) {
            Log.e(TAG, "Exception trying to create proto from parcel", e);
        } catch (IllegalAccessException e) {
            Log.e(TAG, "Exception trying to create proto from parcel", e);
        } catch (InstantiationException e) {
            Log.e(TAG, "Exception trying to create proto from parcel", e);
        } catch (InvalidProtocolBufferNanoException e) {
            Log.e(TAG, "Exception trying to create proto from parcel", e);
        }

        return proto;
    }

    @SuppressWarnings("unchecked")
    @Override
    public T[] newArray(int i) {
        return (T[]) Array.newInstance(mClazz, i);
    }

    static <T extends MessageNano> void writeToParcel(Class<T> clazz, MessageNano message,
            Parcel out) {
        out.writeString(clazz.getName());
        out.writeByteArray(MessageNano.toByteArray(message));
    }
}
