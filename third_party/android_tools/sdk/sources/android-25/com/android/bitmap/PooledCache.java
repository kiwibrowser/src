/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.bitmap;

public interface PooledCache<K, V> {

    V get(K key, boolean incrementRefCount);
    V put(K key, V value);
    void offer(V scrapValue);
    V poll();
    String toDebugString();

    /**
     * Purge existing Poolables from the pool+cache. Usually, this is done when situations
     * change and the items in the pool+cache are no longer appropriate. For example,
     * if the layout changes, the pool+cache may need to hold larger bitmaps.
     *
     * <p/>
     * The existing Poolables will be garbage collected when they are no longer being referenced
     * by other objects.
     */
    void clear();
}
