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

package com.google.android.libraries.feed.host.storage;

import android.support.annotation.IntDef;

/** Status after completion of a commit to storage. */
public class CommitResult {
  @IntDef({Result.SUCCESS, Result.FAILURE})
  @interface Result {
    int SUCCESS = 0;
    int FAILURE = 1;
  }

  public @Result int getResult() {
    return result;
  }

  private final @Result int result;

  // Private constructor - use static instances
  private CommitResult(@Result int result) {
    this.result = result;
  }

  public static final CommitResult SUCCESS = new CommitResult(Result.SUCCESS);
  public static final CommitResult FAILURE = new CommitResult(Result.FAILURE);
}
