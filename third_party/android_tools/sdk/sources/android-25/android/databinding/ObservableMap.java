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

import java.util.Map;

/**
 * A {@link Map} that notifies when items change. This kind of Map may be data bound
 * and have the UI update as the map changes.
 * <p>
 * Implementers must call {@link OnMapChangedCallback#onMapChanged(ObservableMap, Object)} whenever
 * an item is added, changed, or removed.
 * <p>
 * ObservableArrayMap is a convenient implementation of ObservableMap.
 * MapChangeRegistry may help other implementations manage the callbacks.
 * @see Observable
 * @see ObservableList
 */
public interface ObservableMap<K, V> extends Map<K, V> {

    /**
     * Adds a callback to listen for changes to the ObservableMap.
     * @param callback The callback to start listening for events.
     */
    void addOnMapChangedCallback(
            OnMapChangedCallback<? extends ObservableMap<K, V>, K, V> callback);

    /**
     * Removes a previously added callback.
     * @param callback The callback that no longer needs to be notified of map changes.
     */
    void removeOnMapChangedCallback(
            OnMapChangedCallback<? extends ObservableMap<K, V>, K, V> callback);

    /**
     * A callback receiving notifications when an ObservableMap changes.
     */
    abstract class OnMapChangedCallback<T extends ObservableMap<K, V>, K, V> {

        /**
         * Called whenever an ObservableMap changes, including values inserted, deleted,
         * and changed.
         * @param sender The changing map.
         * @param key The key of the value inserted, removed, or changed.
         */
        public abstract void onMapChanged(T sender, K key);
    }
}
