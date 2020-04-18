// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.common.testing;

import com.google.android.libraries.feed.common.functional.Supplier;

/** Utility methods for working with the {@link Supplier} class. */
public class Suppliers {
  /**
   * Instead of {@link #of(T)} returning a lambda, use this inner class to avoid making new classes
   * for each lambda.
   */
  private static class InstancesSupplier<T> implements Supplier<T> {
    private final T instance;

    InstancesSupplier(T instance) {
      this.instance = instance;
    }

    @Override
    public T get() {
      return instance;
    }
  }

  /** Return a {@link Supplier} that always returns the provided instance. */
  public static <T> Supplier<T> of(T instance) {
    return new InstancesSupplier<>(instance);
  }

  /** Prevent instantiation */
  private Suppliers() {}
}
