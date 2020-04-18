// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

/**
 * A function that takes no arguments and returns a value.
 *
 * TODO(sanfin): replace with Java 8 library if we're ever able to use the Java 8 library.
 *
 * @param <T> The result type.
 */
public interface Supplier<T> { public T get(); }
