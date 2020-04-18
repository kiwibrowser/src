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

package com.android.ex.camera2.portability;

import android.graphics.Point;
import android.hardware.Camera;
import android.text.TextUtils;

import java.util.ArrayList;
import java.util.List;

/**
 * An immutable simple size container.
 */
public class Size {
    public static final String DELIMITER = ",";

    /**
     * An helper method to build a list of this class from a list of
     * {@link android.hardware.Camera.Size}.
     *
     * @param cameraSizes Source.
     * @return The built list.
     */
    public static List<Size> buildListFromCameraSizes(List<Camera.Size> cameraSizes) {
        ArrayList<Size> list = new ArrayList<Size>(cameraSizes.size());
        for (Camera.Size cameraSize : cameraSizes) {
            list.add(new Size(cameraSize));
        }
        return list;
    }

    /**
     * A helper method to build a list of this class from a list of {@link android.util.Size}.
     *
     * @param cameraSizes Source.
     * @return The built list.
     */
    public static List<Size> buildListFromAndroidSizes(List<android.util.Size> androidSizes) {
        ArrayList<Size> list = new ArrayList<Size>(androidSizes.size());
        for (android.util.Size androidSize : androidSizes) {
            list.add(new Size(androidSize));
        }
        return list;
    }

    /**
     * Encode List of this class as comma-separated list of integers.
     *
     * @param sizes List of this class to encode.
     * @return encoded string.
     */
    public static String listToString(List<Size> sizes) {
        ArrayList<Integer> flatSizes = new ArrayList<>();
        for (Size s : sizes) {
            flatSizes.add(s.width());
            flatSizes.add(s.height());
        }
        return TextUtils.join(DELIMITER, flatSizes);
    }

    /**
     * Decode comma-separated even-length list of integers into a List of this class.
     *
     * @param encodedSizes encoded string.
     * @return List of this class.
     */
    public static List<Size> stringToList(String encodedSizes) {
        String[] flatSizes = TextUtils.split(encodedSizes, DELIMITER);
        ArrayList<Size> list = new ArrayList<>();
        for (int i = 0; i < flatSizes.length; i += 2) {
            int width = Integer.parseInt(flatSizes[i]);
            int height = Integer.parseInt(flatSizes[i + 1]);
            list.add(new Size(width,height));
        }
        return list;
    }

    private final Point val;

    /**
     * Constructor.
     */
    public Size(int width, int height) {
        val = new Point(width, height);
    }

    /**
     * Copy constructor.
     */
    public Size(Size other) {
        if (other == null) {
            val = new Point(0, 0);
        } else {
            val = new Point(other.width(), other.height());
        }
    }

    /**
     * Constructor from a source {@link android.hardware.Camera.Size}.
     *
     * @param other The source size.
     */
    public Size(Camera.Size other) {
        if (other == null) {
            val = new Point(0, 0);
        } else {
            val = new Point(other.width, other.height);
        }
    }

    /**
     * Constructor from a source {@link android.util.Size}.
     *
     * @param other The source size.
     */
    public Size(android.util.Size other) {
        if (other == null) {
            val = new Point(0, 0);
        } else {
            val = new Point(other.getWidth(), other.getHeight());
        }
    }

    /**
     * Constructor from a source {@link android.graphics.Point}.
     *
     * @param p The source size.
     */
    public Size(Point p) {
        if (p == null) {
            val = new Point(0, 0);
        } else {
            val = new Point(p);
        }
    }

    public int width() {
        return val.x;
    }

    public int height() {
        return val.y;
    }

    @Override
    public boolean equals(Object o) {
        if (o instanceof Size) {
            Size other = (Size) o;
            return val.equals(other.val);
        }
        return false;
    }

    @Override
    public int hashCode() {
        return val.hashCode();
    }

    @Override
    public String toString() {
        return "Size: (" + this.width() + " x " + this.height() + ")";
    }
}
