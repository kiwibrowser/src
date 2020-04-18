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

package com.android.bitmap.drawable;

import android.graphics.drawable.Drawable;

/**
 * {@link Drawable}s that support a parallax effect when drawing should
 * implement this interface to receive the current parallax fraction to use when
 * drawing.
 */
public interface Parallaxable {
    /**
     * @param fraction the vertical center point for the viewport, in the range [0,1]
     */
    void setParallaxFraction(float fraction);
}