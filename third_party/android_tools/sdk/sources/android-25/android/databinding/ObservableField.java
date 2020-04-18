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
package android.databinding;

import java.io.Serializable;

/**
 * An object wrapper to make it observable.
 * <p>
 * Observable field classes may be used instead of creating an Observable object:
 * <pre><code>public class MyDataObject {
 *     public final ObservableField&lt;String> name = new ObservableField&lt;String>();
 *     public final ObservableInt age = new ObservableInt();
 * }</code></pre>
 * Fields of this type should be declared final because bindings only detect changes in the
 * field's value, not of the field itself.
 *
 * @param <T> The type parameter for the actual object.
 * @see android.databinding.ObservableParcelable
 */
public class ObservableField<T> extends BaseObservable implements Serializable {
    static final long serialVersionUID = 1L;
    private T mValue;

    /**
     * Wraps the given object and creates an observable object
     *
     * @param value The value to be wrapped as an observable.
     */
    public ObservableField(T value) {
        mValue = value;
    }

    /**
     * Creates an empty observable object
     */
    public ObservableField() {
    }

    /**
     * @return the stored value.
     */
    public T get() {
        return mValue;
    }

    /**
     * Set the stored value.
     */
    public void set(T value) {
        if (value != mValue) {
            mValue = value;
            notifyChange();
        }
    }
}
