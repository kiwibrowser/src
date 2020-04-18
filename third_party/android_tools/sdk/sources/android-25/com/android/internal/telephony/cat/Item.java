/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony.cat;

import android.graphics.Bitmap;
import android.os.Parcel;
import android.os.Parcelable;

/**
 * Represents an Item COMPREHENSION-TLV object.
 *
 * {@hide}
 */
public class Item implements Parcelable {
    /** Identifier of the item. */
    public int id;
    /** Text string of the item. */
    public String text;
    /** Icon of the item */
    public Bitmap icon;

    public Item(int id, String text) {
        this(id, text, null);
    }

    public Item(int id, String text, Bitmap icon) {
        this.id = id;
        this.text = text;
        this.icon = icon;
    }

    public Item(Parcel in) {
        id = in.readInt();
        text = in.readString();
        icon = in.readParcelable(null);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(id);
        dest.writeString(text);
        dest.writeParcelable(icon, flags);
    }

    public static final Parcelable.Creator<Item> CREATOR = new Parcelable.Creator<Item>() {
        @Override
        public Item createFromParcel(Parcel in) {
            return new Item(in);
        }

        @Override
        public Item[] newArray(int size) {
            return new Item[size];
        }
    };

    @Override
    public String toString() {
        return text;
    }
}
