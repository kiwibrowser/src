// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import java.util.Iterator;
import java.util.Stack;

/**
 * Utility methods for working with Iterables and Iterators.
 */
public class Itertools {
    /**
     * Iterate the given Iterable in reverse.
     *
     * This is inefficient, as the entire given Iterable needs to be iterated before the result can
     * be iterated, and O(n) memory needs to be allocated out-of-place to store the stack. However,
     * for small, simple tasks, like iterating an ObserverList in reverse, this is a useful way to
     * concisely iterate a container in reverse order in a for-each loop.
     *
     * Example:
     *
     *      List<String> lines = ...;
     *      // Prints lines in reverse order that they were added to the list.
     *      for (String line : Itertools.reverse(lines)) {
     *          Log.i(TAG, line);
     *      }
     */
    public static <T> Iterable<T> reverse(Iterable<? extends T> iterable) {
        return (() -> {
            // Lazily iterate `iterable` only after reverse()'s `iterator()` method is called.
            Stack<T> stack = new Stack<T>();
            for (T item : iterable) {
                stack.push(item);
            }
            return new Iterator<T>() {
                @Override
                public boolean hasNext() {
                    return !stack.empty();
                }
                @Override
                public T next() {
                    return stack.pop();
                }
            };
        });
    }
}
