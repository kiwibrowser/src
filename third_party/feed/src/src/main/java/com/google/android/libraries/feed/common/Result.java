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

package com.google.android.libraries.feed.common;


/** Wrapper that allows callbacks to return a value as well as whether the call was successful. */
public class Result<T> {

  /*@Nullable*/ private final T value;
  private final boolean isSuccessful;

  private Result(/*@Nullable*/ T value, boolean isSuccessful) {
    this.value = value;
    this.isSuccessful = isSuccessful;
  }

  public static <T> Result<T> success(T value) {
    return new Result<>(value, /* isSuccessful= */ true);
  }

  public static <T> Result<T> failure() {
    return new Result<>(null, false);
  }

  /** Retrieves the value for the result. */
  public T getValue() {
    if (!isSuccessful) {
      throw new IllegalStateException("Cannot retrieve value for failed result");
    }
    return Validators.checkNotNull(value);
  }

  public boolean isSuccessful() {
    return isSuccessful;
  }
}
