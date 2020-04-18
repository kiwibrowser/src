/*
 * Copyright (C) 2014 The Android Open Source Project
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

/**
 * Observable classes provide a way in which data bound UI can be notified of changes.
 * {@link ObservableList} and {@link ObservableMap} also provide the ability to notify when
 * changes occur. ObservableField, ObservableParcelable, ObservableBoolean, ObservableByte,
 * ObservableShort, ObservableInt, ObservableLong, ObservableFloat, and ObservableDouble provide
 * a means by which properties may be notified without implementing Observable.
 * <p>
 * An Observable object should notify the {@link OnPropertyChangedCallback} whenever
 * an observed property of the class changes.
 * <p>
 * The getter for an observable property should be annotated with {@link Bindable}.
 * <p>
 * Convenience class BaseObservable implements this interface and PropertyChangeRegistry
 * can help classes that don't extend BaseObservable to implement the listener registry.
 */
public interface Observable {

    /**
     * Adds a callback to listen for changes to the Observable.
     * @param callback The callback to start listening.
     */
    void addOnPropertyChangedCallback(OnPropertyChangedCallback callback);

    /**
     * Removes a callback from those listening for changes.
     * @param callback The callback that should stop listening.
     */
    void removeOnPropertyChangedCallback(OnPropertyChangedCallback callback);

    /**
     * The callback that is called by Observable when an observable property has changed.
     */
    abstract class OnPropertyChangedCallback {

        /**
         * Called by an Observable whenever an observable property changes.
         * @param sender The Observable that is changing.
         * @param propertyId The BR identifier of the property that has changed. The getter
         *                   for this property should be annotated with {@link Bindable}.
         */
        public abstract void onPropertyChanged(Observable sender, int propertyId);
    }
}
