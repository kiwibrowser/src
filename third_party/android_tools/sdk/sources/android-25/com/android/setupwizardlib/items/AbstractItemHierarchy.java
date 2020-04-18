/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.setupwizardlib.items;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;

import com.android.setupwizardlib.R;

import java.util.ArrayList;

/**
 * An abstract item hierarchy; provides default implementation for ID and observers.
 */
public abstract class AbstractItemHierarchy implements ItemHierarchy {

    private ArrayList<Observer> mObservers = new ArrayList<>();
    private int mId = 0;

    public AbstractItemHierarchy() {
    }

    public AbstractItemHierarchy(Context context, AttributeSet attrs) {
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.SuwAbstractItem);
        mId = a.getResourceId(R.styleable.SuwAbstractItem_android_id, 0);
        a.recycle();
    }

    public void setId(int id) {
        mId = id;
    }

    public int getId() {
        return mId;
    }

    @Override
    public void registerObserver(Observer observer) {
        mObservers.add(observer);
    }

    @Override
    public void unregisterObserver(Observer observer) {
        mObservers.remove(observer);
    }

    public void notifyChanged() {
        for (Observer observer : mObservers) {
            observer.onChanged(this);
        }
    }
}
