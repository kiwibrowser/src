/*
 * Copyright (C) 2007 The Android Open Source Project
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

public class TextMessage implements Parcelable {
    public String title = "";
    public String text = null;
    public Bitmap icon = null;
    public boolean iconSelfExplanatory = false;
    public boolean isHighPriority = false;
    public boolean responseNeeded = true;
    public boolean userClear = false;
    public Duration duration = null;

    TextMessage() {
    }

    private TextMessage(Parcel in) {
        title = in.readString();
        text = in.readString();
        icon = in.readParcelable(null);
        iconSelfExplanatory = in.readInt() == 1 ? true : false;
        isHighPriority = in.readInt() == 1 ? true : false;
        responseNeeded = in.readInt() == 1 ? true : false;
        userClear = in.readInt() == 1 ? true : false;
        duration = in.readParcelable(null);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(title);
        dest.writeString(text);
        dest.writeParcelable(icon, 0);
        dest.writeInt(iconSelfExplanatory ? 1 : 0);
        dest.writeInt(isHighPriority ? 1 : 0);
        dest.writeInt(responseNeeded ? 1 : 0);
        dest.writeInt(userClear ? 1 : 0);
        dest.writeParcelable(duration, 0);
    }

    public static final Parcelable.Creator<TextMessage> CREATOR = new Parcelable.Creator<TextMessage>() {
        @Override
        public TextMessage createFromParcel(Parcel in) {
            return new TextMessage(in);
        }

        @Override
        public TextMessage[] newArray(int size) {
            return new TextMessage[size];
        }
    };

    @Override
    public String toString() {
        return "title=" + title + " text=" + text + " icon=" + icon +
            " iconSelfExplanatory=" + iconSelfExplanatory + " isHighPriority=" +
            isHighPriority + " responseNeeded=" + responseNeeded + " userClear=" +
            userClear + " duration=" + duration;
    }
}
